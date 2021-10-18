#include <Arduino.h>

unsigned int convertToMilliAmps(unsigned int analogReadMeasurement);
unsigned int getMilliAmps();

const unsigned long CHECK_INTERVAL_MILLIS = 5000;
const unsigned long RELAY_SHUTOFF_LAG_MILLIS = 5000; // in practice, this will be much longer than 5s
const float PEAK_TO_PEAK_RMS_CONVERSION = 0.3536;
const int SENSOR_PIN = A0;
const int RELAY_PIN = 3;
const int CURRENT_SAMPLE_DURATION_MILLIS = 100;
const int CURRENT_SENSOR_MAX_CURRENT_MILLIAMPS = 20000;
const int CURRENT_SENSOR_RESOLUTION = 1023;
const int TRIGGER_CURRENT_MILLIAMPS = 100;
int ledState = LOW;
unsigned long previousMillis = 0;
unsigned long turnRelayOffAt = 0;



void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= CHECK_INTERVAL_MILLIS) {
    unsigned int milliAmps = getMilliAmps();
    Serial.print("measured milliamps: ");
    Serial.print(milliAmps);

    previousMillis = currentMillis;
    if (milliAmps >= TRIGGER_CURRENT_MILLIAMPS) {
      ledState = HIGH;
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("measured more than 100 mA!");
    } else {
      bool wasRelayJustOn = ledState == HIGH;
      ledState = LOW;
      // here's where we'd turn off the light/relay.
      // here's where we'd blink the light 
      Serial.println("measured less than 100 mA :(");
      // if we were previously on, wait X seconds before turning off the relay pin.
      if (wasRelayJustOn) {
        delay(RELAY_SHUTOFF_LAG_MILLIS);
      }
      digitalWrite(RELAY_PIN, LOW);
    }
    digitalWrite(LED_BUILTIN, ledState);
  }
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

  Serial.print("max: ");
  Serial.print(maxAnalogReadObserved);
  Serial.print(" min: ");
  Serial.print(minAnalogReadObserved);
  Serial.print(" samples collected: ");
  Serial.print(samplesCollected);
  Serial.print("\n");
  return convertToMilliAmps(max(maxAnalogReadObserved - minAnalogReadObserved, 0));
}

// the analogRead value will be between 0 and 1023. of analog values
// corresponds to a current range of 0 to 20,000 mA.
// (observed analog measurement/maximum analog measurement) * (20,000 mA for maximum analog measurement) * RMS conversion value
unsigned int convertToMilliAmps(unsigned int analogReadMeasurement) {
  return (unsigned int) ((analogReadMeasurement / 1023.0) * CURRENT_SENSOR_MAX_CURRENT_MILLIAMPS * PEAK_TO_PEAK_RMS_CONVERSION);
}