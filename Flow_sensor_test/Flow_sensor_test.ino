#include "Adafruit_MCP23017.h"

Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;

const uint8_t N_TRAYS = 8;

// MCP23017 (0) pin connections.
// The sensor connections are in the boardDefinitions.h file.
const uint8_t PUMP_PIN[] = {8, 9, 10, 11, 12, 15, 14, 13};  // GPIOB pins, complete bank.

// MCP23017 (1) pin connections.
const uint8_t FLOW_PIN[] = {7, 6, 4, 5, 2, 3, 0, 1};        // GPIOA pins.
//const uint8_t TRAY_PIN[] = {9, 8, 10, 11, 12, 13, 14, 15};
const uint8_t TRAY_PIN[] = {1, 0, 2, 3, 4, 5, 6, 7};        // GPIOB pins.
uint8_t flowSensorPins;

// reverse mapping: MAP[pin] = sensor
// Flow sensors are all GPIOA, tray sensors are all GPIOB.
const uint8_t FLOW_SENSOR_MAP[] = {7, 8, 5, 6, 3, 4, 2, 1};
const uint8_t TRAY_SENSOR_MAP[] = {2, 1, 3, 4, 5, 6, 7, 8};


uint32_t flowSensorCount[N_TRAYS];
bool flowSensorPinStatus[N_TRAYS];
uint8_t trayStatus;
uint8_t oldFlowSensorPins;

enum RunState {
  PUMP_IDLE,
  PUMP_RUNNING
};
RunState runState = PUMP_IDLE;

void setup() {
  Serial.begin(115200);
  mcp0.begin(0);
  mcp1.begin(1);

  for (uint8_t i = 0; i < N_TRAYS; i++ ) {
    mcp1.pinMode(FLOW_PIN[i], INPUT);
    mcp1.pullUp(FLOW_PIN[i], true);
    mcp1.pinMode(TRAY_PIN[i], INPUT);
    mcp1.pullUp(TRAY_PIN[i], true);
    mcp0.pinMode(PUMP_PIN[i], OUTPUT);
  }
  oldFlowSensorPins = mcp1.readGPIO(0);
  Serial.println();
  Serial.println("Enter pump number (1-8) to start calibrating that pump.");
}

void loop() {
  static uint8_t pump;
  if (Serial.available()) {
    char c = Serial.read();
    switch (runState) {
      case PUMP_IDLE:
        if (c > '0' && c < '9') {
          pump = c - '0' - 1;
          runState = PUMP_RUNNING;
          Serial.print("Doing calibration run on pump ");
          Serial.print(pump + 1);
          Serial.println(".");
          Serial.println("Enter command s to stop.");
          flowSensorCount[pump] = 0;
          mcp0.digitalWrite(PUMP_PIN[pump], HIGH);
          runState = PUMP_RUNNING;
        }
        break;

      case PUMP_RUNNING:
        if (c == 's') {
          mcp0.digitalWrite(PUMP_PIN[pump], LOW);
          Serial.print("Calibration of pump ");
          Serial.print(pump);
          Serial.println(" completed.");
          Serial.print("Flow sensor count: ");
          Serial.println(flowSensorCount[pump]);
          Serial.println();
          Serial.println("Enter pump number (1-8) to start calibrating that pump.");
          runState = PUMP_IDLE;
        }
        break;
    }
    while (Serial.available()) Serial.read();
  }

  // Read the status of the flow sensors.
  uint8_t flowSensorPins = mcp1.readGPIO(0);                // 0 = GPIOA, 1 = GPIOB.
  trayStatus = mcp1.readGPIO(1);                            // Where the trays are connected.
  if (flowSensorPins != oldFlowSensorPins) {                // One or more sensors changed status.
    for (uint8_t i = 0; i < 8; i++) {
      if (bitRead(flowSensorPins, FLOW_PIN[i]) ^ bitRead(oldFlowSensorPins, FLOW_PIN[i])) {
        flowSensorCount[i]++;                                 // Count the pulse of that sensor.
      }
    }
    oldFlowSensorPins = flowSensorPins;
  }
}
