///#define FRIDGE_A
#define FRIDGE_B

//#define OPTION1                                             // Only access point, no connection to other networks.
//#define OPTION2                                             // AP & STA; try to connect STA first; then set up AP. wifiMulti. No autoconnect.
//#define OPTION3                                             // AP & STA; try to connect STA first; then set up AP. wifiMulti. Enables autoconnect.
#define OPTION4                                             // AP & STA; try to connect STA first; then set up AP. Single network. Disables autoconnect, full disconnect on detection of network loss.
//#define OPTION5                                             // AP & STA; try to connect STA first; then set up AP. Single network. Enables autoconnect.

// System includes
#include <ESP8266WiFi.h>
#include <Adafruit_MCP23017.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <DS1603L.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SoftwareSerial.h>

#if defined(OPTION1) || defined(OPTION2) || defined (OPTION3)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#endif
//#include <ESP8266mDNS.h>

/****************************************************************************************************
   HydroMonitor library: create the sensors and include the various required libraries for them.
*/
#include <HydroMonitorECSensor.h>
HydroMonitorECSensor ECSensor;
#include <HydroMonitorWaterTempSensor.h>
HydroMonitorWaterTempSensor waterTempSensor;
#ifdef USE_WATERLEVEL_SENSOR
#include <HydroMonitorWaterLevelSensor.h>
HydroMonitorWaterLevelSensor waterLevelSensor;
#endif
#include <HydroMonitorpHSensor.h>
HydroMonitorpHSensor pHSensor;
#include <HydroMonitorDrainage.h>
HydroMonitorDrainage drainage;

/***************************************************************************************************
   HydroMonitor library: the various other peripherals.
*/
#include <HydroMonitorFertiliser.h>
HydroMonitorFertiliser fertiliser;
#include <HydroMonitorpHMinus.h>
HydroMonitorpHMinus pHMinus;
#include <HydroMonitorReservoir.h>
HydroMonitorReservoir reservoir;

/***************************************************************************************************
   HydroMonitor library: the networking functions.
*/
#include <HydroMonitorNetwork.h>
HydroMonitorNetwork network;
#include <HydroMonitorLogging.h>
HydroMonitorLogging logging;
ESP8266WebServer server(80);
bool APIRequest;                                              // To flag the handler it's coming through the API.

/***************************************************************************************************
   HydroMonitor library: various core functions and settings.
*/
#include <HydroMonitorCore.h>
HydroMonitorCore core;
#include <HydroMonitorGrowingParameters.h>
HydroMonitorGrowingParameters parameters;

// The various variables containing the data from the sensors.
HydroMonitorCore::SensorData sensorData;

#define USE_OTA

const uint8_t TRAYS = 8;

// Port expanders and pin connections.
Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
// MCP23017 (0) pin connections.
// More sensor and peripheral connections are in the boardDefinitions.h file.
const uint8_t DOOR_PIN = 4;                                 // Input: whether the door is open or closed.
const uint8_t LEVEL_LIMIT_PIN = 5;                          // Input: water level limit switch. Solenoid won't close unless this one is closed.
const uint8_t WIFI_LED_PIN = 6;                             // Output: the WiFi indicator LED.
const uint8_t PUMP_PIN[TRAYS] = {8, 9, 10, 11, 12, 15, 14, 13}; // GPIOB pins, complete bank.

// MCP23017 (1) pin connections.
const uint8_t FLOW_PIN[TRAYS] = {7, 6, 4, 5, 2, 3, 0, 1};   // GPIOA pins, complete bank.
const uint8_t TRAY_PIN[TRAYS] = {9, 8, 10, 11, 12, 13, 14, 15}; // GPIOB pins, complete bank.

//Direct MCU connections.

// OneWire bus - the DS18B20 temperature sensor.
const uint8_t ONE_WIRE_BUS = 14;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// The DS1603L water level sensor.
//const byte txPin = 16;                                      // tx of the Arduino to rx of the sensor
//const byte rxPin = 2;                                       // rx of the Arduino to tx of the sensor
//SoftwareSerial sensorSerial(rxPin, txPin);
//DS1603L ds1603l (sensorSerial);
DS1603L ds1603l (Serial);
#endif

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V2_H
// MCP23017 (0) pin connections: the water pumps and several peripherals.
// The sensor and peripheral connections (pins 0-7) are in the boardDefinitions.h file.
//const uint8_t LEVEL_LIMIT_PIN = 1;                          // Input: water level limit switch. Solenoid won't close unless this one is closed.
const uint8_t PUMP_PIN[TRAYS] = {8, 9, 10, 11, 12, 13, 14, 15}; // GPIOB pins, complete bank.

// MCP23017 (1) pin connections.
const uint8_t FLOW_PIN[TRAYS] = {7, 6, 4, 5, 2, 3, 0, 1};   // GPIOA pins, complete bank.
const uint8_t TRAY_PIN[TRAYS] = {9, 8, 10, 11, 12, 13, 14, 15}; // GPIOB pins, complete bank.

//Direct MCU connections.
#ifdef USE_DS18B20
// OneWire bus - the DS18B20 temperature sensor.
const uint8_t ONE_WIRE_BUS = 14;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
#endif

// MCP port expanders reset. Must be driven HIGH for normal operations.
const uint8_t MCP_RESET = 0;

#ifdef USE_ISOLATED_SENSOR_BOARD
// The Serial connection for the pH/EC/water temp sensor.
const uint8_t TX2 = 12;
const uint8_t RX2 = 13;
SoftwareSerial sensorSerial(RX2, TX2);

#include <HydroMonitorIsolatedSensorBoard.h>
HydroMonitorIsolatedSensorBoard isolatedSensorBoard;
#endif
#endif

// Create array of available sensors using the HydroMonitorSensorBase class with various virtual entries.
HydroMonitorSensorBase* sensors[] = {
  &ECSensor,
  &waterTempSensor,
#ifdef USE_WATERLEVEL_SENSOR
  &waterLevelSensor,
#endif
  &pHSensor,
#ifdef USE_ISOLATED_SENSOR_BOARD
  &isolatedSensorBoard,
#endif
};


uint32_t flowSensorCount[TRAYS];                            // Number of ticks for this tray to be full of water.
bool flowSensorPinStatus[TRAYS];                            // The current pin state of this flow sensor.
uint16_t trayPresence;                                      // 16 bits: presence of the 8 trays (we use bit 8-15 for pin 8-15).
uint8_t oldFlowSensorPins;                                  // 8 bits: stored status of the flow sensor pins.

// The timing variables.
uint32_t lastSendData = -REFRESH_DATABASE;
uint32_t lastNtpUpdateTime = -REFRESH_NTP;
bool updatingTime = false;

// The possible state of the tray's program.
enum ProgramState {
  PROGRAM_UNSET,                                            // No program set.
  PROGRAM_SET,                                              // Crop has been set.
  PROGRAM_START,                                            // Program is starting.
  PROGRAM_START_WATERING,                                   // Program start: running pump for initial watering.
  PROGRAM_START_PAUSED,                                     // Program paused during start-up stage.
  PROGRAM_RUNNING,                                          // Program in progress.
  PROGRAM_COMPLETE,                                         // Program complete - but if tray is still present, continue as usual.
  PROGRAM_PAUSED,                                           // Program on hold - no lights, no watering.
  PROGRAM_ERROR                                             // Program on hold - error detected.
};

// Possibilities for the watering frequency, in times daily.
const uint8_t N_FREQUENCIES = 8;
const uint8_t WATERING_FREQUENCIES[] = {0, 1, 2, 3, 4, 6, 8, 12};

// A struct holding the info of a tray.
// Reserve 16 bytes in the EEPROM storage for each tray; the remaining bytes are for future use.
struct TrayInfo {
  ProgramState programState;                                // The current status of the program.
  uint8_t cropId;                                           // The crop id (also indicates the relevant program).
  uint32_t startTime;                                       // The start time of the current program in seconds since epoch.
#ifdef USE_GROWLIGHT
  uint8_t darkDays;                                         // The number of days the tray remains dark.
#endif
  uint8_t totalDays;                                        // The number of days the program is to run.
  uint8_t wateringFrequency;                                // The number of times a day the crop needs to be watered.
};
TrayInfo trayInfo[TRAYS];
bool trayInfoChanged;

// A struct to hold all info of a crop.
struct Crop {
  uint8_t id;
  char crop[32];
  char description[201];
  uint8_t darkDays;                                         // The number of days the tray remains dark.
  uint8_t totalDays;                                        // The number of days the program is to run.
  uint16_t wateringFrequency;                               // The interval between two mistings in minutes.
};
Crop getCropData(uint8_t id);                               // Prototype here to keep the compiler happy.

// A struct to hold just crop ID and name.
struct CropId {
  uint8_t cropId;
  char crop[32];
};

// An array to hold a complete list of crop data (note: uses 33 bytes per element).
const uint8_t N_CROPS = 20;
CropId cropId[N_CROPS];

// The watering.
uint8_t currentWateringTray = TRAYS;
uint16_t wateringDuration[TRAYS];
uint32_t wateringStartTime;
uint16_t waterFlowSensorTicks[TRAYS];                       // Number of ticks this water flow sensor has recorded so far.
uint32_t lastFlowCheckTime[TRAYS];                          // When we last checked for erroneous flow for this tray.

enum WateringState {
  WATERING_IDLE,                                            // No watering happening now.
  WATERING_NEEDED,                                          // Tray needs watering - pending.
  WATERING,                                                 // Watering in progress: pump runs.
  DRAINING,                                                 // Watering completed: tray drains.
  CALIBRATING                                               // Calibration of this tray in progress.
};
WateringState wateringState[TRAYS];                         // Keep this out of the TrayInfo, it's better not stored in EEPROM.

uint32_t wateringTime[TRAYS];
bool pumpConfirmedRunning;

const uint32_t DRAIN_TIME = 5 * 60 * 1000;                  // Allow 5 minutes for the tray to drain back in the reservoir.
uint8_t lastWateredTray;

char buff[MAX_MESSAGE_SIZE];                                // General purpose string for use as temporary buffer, a.o. for creating messages for the logging system.

// EEPROM addresses.
// Some have been defined already in HydroMonitorCore! Can use space starting at FREE_EEPROM.
const uint16_t TRAYINFO_EEPROM = FREE_EEPROM;
const uint16_t FLOWSENSOR_COUNT_EEPROM = TRAYINFO_EEPROM + sizeof(trayInfo);

// Fridge access point.
#ifdef FRIDGE_A
const char* local_name = "fridgea";
const char* ap_ssid = "Fridge A";
const char* ota_password = "fridgea";
#elif defined(FRIDGE_B)
const char* local_name = "fridgeb";
const char* ap_ssid = "Fridge B";
const char* ota_password = "fridgeb";
#endif

const char* ap_password = "fridgewifi";

#ifdef USE_OTA
/* BEGIN OTA (part 1 of 3) */
#include <ArduinoOTA.h>
#endif

// Blink the WiFi LED while we're trying to connect.
extern "C" {
#include "user_interface.h"
}

///***********************************************************************************************************
//   configModeCallback()
//*/
//void configModeCallback(WiFiManager *myWiFiManager) {
//}

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
void writeLed(bool ledState) {
  mcp0.digitalWrite(WIFI_LED_PIN, ledState);
}

bool ledState = HIGH;
void blinkTimerCallback(void *pArg) {
  ledState = !ledState;
  writeLed(ledState);
}
os_timer_t blinkTimer;
#endif
