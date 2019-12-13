// This code belongs to the isolated Arduino on Williams Fridge Control Board V.2

#include <SoftwareSerial.h>
#include <EEPROM.h>

//#define DEBUG

const uint8_t rxPin = 6;
const uint8_t txPin = 5;
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);
const uint8_t PH_PIN = 1;                                   // A1, ADC1

const uint8_t CAPPOS = 2;
const uint8_t CAPNEG = 4;
const uint8_t ECPIN = 3;

uint16_t vRef;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  mySerial.begin(2400);                                     // Nice and slow or the ESP8266 can't follow...
  initECSensor();
  initpHSensor();
}

uint16_t i = 0;
uint32_t lastTransmitData;
const uint32_t transmitInterval = 3000;

// Readings transmitted as:
// <Eec,Pph>
// with ec as discharge cycles (0-65535)
// ph as analog reading *32 (0-32767)

uint32_t lastPrint;
const uint32_t printInterval = 2000;

void loop() {
  uint32_t ECReading, pHReading;
  if (millis() - lastTransmitData > transmitInterval) {
    lastTransmitData += transmitInterval;
    pHReading = readpHSensor();
    ECReading = readECSensor();
    char s[22];
    sprintf_P(s, PSTR("<E%d,P%d>"), int(ECReading), pHReading);
    mySerial.print(s);
#ifdef DEBUG
    Serial.print(F("Test: "));
    Serial.print(i++);
    Serial.print(F(", discharge cycles: "));
    Serial.print(ECReading);
    Serial.print(F(", pH reading: "));
    Serial.print(pHReading);
    Serial.print(F(". Transmitting: "));
    Serial.println(s);
#endif
  }
}
