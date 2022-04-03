#include <Arduino.h>
#include <ArduinoLog.h>

enum RelayState { on, shutting_down, off };

unsigned int convertToMilliAmps(unsigned int analogReadMeasurement);
unsigned int getMilliAmps();
uint8_t ledPinValueFromRelayState(RelayState relayState, unsigned long currentMillis);
uint8_t relayPinValueFromRelayState(RelayState relayState);


const unsigned long CHECK_INTERVAL_MILLIS = 5000;
const unsigned long AMPERAGE_FOLLOWER_RELAY_SHUTOFF_LAG_MILLIS = 5000; // in practice, this will be much longer than 5s
const unsigned long AMPERAGE_SHUTOFF_EVENT_RELAY_SHUTOFF_LAG_MILLIS = 500;
const float PEAK_TO_PEAK_RMS_CONVERSION = 0.3536;
const int SENSOR_PIN = A0;
const int AMPERAGE_FOLLOWER_RELAY_PIN = 3;
const int AMPERAGE_FOLLOWER_LED_PIN = 2;
const int AMPERAGE_SHUTOFF_EVENT_RELAY_PIN = 4;
const int AMPERAGE_SHUTOFF_EVENT_LED_PIN = 5;
const int CURRENT_SAMPLE_DURATION_MILLIS = 100;
const int CURRENT_SENSOR_MAX_CURRENT_MILLIAMPS = 20000;
const int CURRENT_SENSOR_RESOLUTION = 1023;
const int TRIGGER_CURRENT_MILLIAMPS = 250;

RelayState amperageFollowerRelayState = off;
RelayState amperageShutoffEventRelayState = off;

unsigned long previousMillis = 0;
unsigned long turnAmperageFollowerRelayOffAt = 0;
unsigned long turnAmperageShutoffEventRelayOffAt = 0;



void setup() {
  Serial.begin(9600);
  pinMode(AMPERAGE_FOLLOWER_RELAY_PIN, OUTPUT);
  pinMode(AMPERAGE_FOLLOWER_LED_PIN, OUTPUT);
  pinMode(AMPERAGE_SHUTOFF_EVENT_RELAY_PIN, OUTPUT);
  pinMode(AMPERAGE_SHUTOFF_EVENT_LED_PIN, OUTPUT);
  
  digitalWrite(AMPERAGE_FOLLOWER_RELAY_PIN, LOW);
  digitalWrite(AMPERAGE_FOLLOWER_LED_PIN, LOW);
  digitalWrite(AMPERAGE_SHUTOFF_EVENT_RELAY_PIN, LOW);
  digitalWrite(AMPERAGE_SHUTOFF_EVENT_LED_PIN, LOW);

  Log.begin(LOG_LEVEL_TRACE, &Serial);
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= CHECK_INTERVAL_MILLIS) {
    unsigned int milliAmps = getMilliAmps();
    Log.traceln("measured milliamps: %d", milliAmps);
    currentMillis = millis();

    previousMillis = currentMillis;
    if (milliAmps >= TRIGGER_CURRENT_MILLIAMPS) {
      amperageFollowerRelayState = RelayState::on;
      amperageShutoffEventRelayState = RelayState::off;
      Log.traceln("measured %d, more than 100 mA!", milliAmps);
    } else {
      Log.traceln("measured %d, less than 100 mA :(", milliAmps);
      // were we previously on or shutting down? 
      if (amperageFollowerRelayState == RelayState::on) {
        amperageFollowerRelayState = RelayState::shutting_down;
        turnAmperageFollowerRelayOffAt = currentMillis + AMPERAGE_FOLLOWER_RELAY_SHUTOFF_LAG_MILLIS;
        amperageShutoffEventRelayState = RelayState::on;
        turnAmperageShutoffEventRelayOffAt = currentMillis + AMPERAGE_SHUTOFF_EVENT_RELAY_SHUTOFF_LAG_MILLIS; 
      }
    }
  }
  
  if (amperageFollowerRelayState == RelayState::shutting_down
      && currentMillis > turnAmperageFollowerRelayOffAt) {
      amperageFollowerRelayState = RelayState::off;
  }
  
  if (currentMillis > turnAmperageShutoffEventRelayOffAt
      && (amperageFollowerRelayState == RelayState::off || amperageFollowerRelayState == RelayState::shutting_down)) {
    amperageShutoffEventRelayState = RelayState::off;
  }


  digitalWrite(AMPERAGE_FOLLOWER_RELAY_PIN, relayPinValueFromRelayState(amperageFollowerRelayState));
  digitalWrite(AMPERAGE_FOLLOWER_LED_PIN, ledPinValueFromRelayState(amperageFollowerRelayState, currentMillis));
  digitalWrite(AMPERAGE_SHUTOFF_EVENT_RELAY_PIN, relayPinValueFromRelayState(amperageShutoffEventRelayState));
  digitalWrite(AMPERAGE_SHUTOFF_EVENT_LED_PIN, ledPinValueFromRelayState(amperageShutoffEventRelayState, currentMillis));
}

unsigned int getMilliAmps() {
  unsigned int maxAnalogReadObserved = 0;
  unsigned int minAnalogReadObserved = 1024;
  unsigned long sampleStartTime = millis();
  unsigned int samplesCollected = 0;
  
  while (millis() < sampleStartTime + CURRENT_SAMPLE_DURATION_MILLIS) {
  // for (int i = 0; i < 500; i++) {
    unsigned int sampleValue = analogRead(SENSOR_PIN);
    if (sampleValue > maxAnalogReadObserved) {
      maxAnalogReadObserved = sampleValue;
    } else if (sampleValue < minAnalogReadObserved) {
      minAnalogReadObserved = sampleValue;
    }
    samplesCollected++;
  }
  unsigned int milliAmps = convertToMilliAmps(max(maxAnalogReadObserved - minAnalogReadObserved, 0));
  Log.traceln(
    "min: %d, max: %d, sample count: %d, milliamp conversion: %d",
    minAnalogReadObserved,
    maxAnalogReadObserved,
    samplesCollected,
    milliAmps
  );
  return milliAmps;
}

// the analogRead value will be between 0 and 1023. of analog values
// corresponds to a current range of 0 to 20,000 mA.
// (observed analog measurement/maximum analog measurement) * (20,000 mA for maximum analog measurement) * RMS conversion value
unsigned int convertToMilliAmps(unsigned int analogReadMeasurement) {
  return (unsigned int) ((analogReadMeasurement / 1023.0) * CURRENT_SENSOR_MAX_CURRENT_MILLIAMPS * PEAK_TO_PEAK_RMS_CONVERSION);
}

uint8_t ledPinValueFromRelayState(RelayState relayState, unsigned long currentMillis) {
  if (relayState == RelayState::off) {
    return LOW;
  } else if (relayState == RelayState::on) {
    return HIGH;
  }

  unsigned int millis_part = currentMillis % 1000;
  if (millis_part < 250 || (millis_part > 500 && millis_part < 750)) {
    return HIGH;
  }
  return LOW;
}

uint8_t relayPinValueFromRelayState(RelayState relayState) {
  if (relayState == RelayState::on || relayState == RelayState::shutting_down) {
    return HIGH;
  }
  return LOW;
}