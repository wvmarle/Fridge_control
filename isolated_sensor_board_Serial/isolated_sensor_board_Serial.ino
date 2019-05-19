// This code belongs to the isolated Arduino on Williams Fridge Control Board V.2

#include <SoftwareSerial.h>
#include <EEPROM.h>

#define DEBUG

const uint8_t rxPin = 6;
const uint8_t txPin = 5;
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);
const uint8_t PH_PIN = 1;                                   // A1, ADC1
//const uint8_t TEMP_PIN = 0;                                 // A0, ADC0

const uint8_t CAPPOS = 2;
const uint8_t CAPNEG = 4;
const uint8_t ECPIN = 3;

uint16_t vRef;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  mySerial.begin(9600);
  initECSensor();
  initpHSensor();
//  initTemperatureSensor();
//  analogReference(INTERNAL);
//  EEPROM.get(0, vRef);
//  if (vRef > 5000) {
//    vRef = 1100;
//  }
//#ifdef DEBUG
//  else {
//    Serial.print(F("Found calibrated ADC reference voltage: "));
//    Serial.println(vRef);
//  }
//#endif
}

uint16_t i = 0;
uint32_t lastTransmitData;
const uint32_t transmitInterval = 3000;

// Readings transmitted as:
// <Eec,Ttemp,Pph>
// with ec as discharge cycles (0-65535)
// temp as *10 degrees C (0 - 1000 for 0 - 100.0 C)
// ph as analog reading *32 (0-32767)

uint32_t lastPrint;
const uint32_t printInterval = 2000;

void loop() {
  uint32_t ECReading, pHReading;
//  float temperature;                                        // The temperature in deg. C.
  if (millis() - lastTransmitData > transmitInterval) {
    lastTransmitData += transmitInterval;
    pHReading = readpHSensor();
//    temperature = readTemperatureSensor();
    ECReading = readECSensor();
    char s[22];
//    sprintf_P(s, PSTR("<E%d,T%d,P%d>"), int(ECReading), int(temperature * 10), pHReading);
    sprintf_P(s, PSTR("<E%d,T%d,P%d>"), int(ECReading), 0, pHReading);
#ifdef DEBUG
    Serial.print(F("Test: "));
    Serial.print(i++);
    Serial.print(F(", discharge cycles: "));
    Serial.print(ECReading);
//    Serial.print(F(", temp: "));
//    Serial.print(temperature);
    Serial.print(F(", pH reading: "));
    Serial.print(pHReading);
    Serial.print(F(". Transmitting: "));
    Serial.println(s);
#endif
    mySerial.print(s);
  }
}