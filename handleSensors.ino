void initSensors() {
  parameters.begin(&sensorData, &logging);
  pHMinus.begin(&sensorData, &logging, &mcp0);

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
  ECSensor.begin(&sensorData, &logging);
  pHSensor.begin(&sensorData, &logging);
  fertiliser.begin(&sensorData, &logging, &mcp0);
#ifdef USE_DS1603L
  waterLevelSensor.begin(&sensorData, &logging, &ds1603l);
#elif defined (USE_HCSR04)
  waterLevelSensor.begin(&sensorData, &logging);
#endif

#elif defined(HYDROMONITOR_WILLIAMS_FRIDGE_V2_H)
  sensorSerial.begin(2400);
  isolatedSensorBoard.begin(&sensorData, &logging, &sensorSerial);
  ECSensor.begin(&sensorData, &logging);
  pHSensor.begin(&sensorData, &logging);
  fertiliser.begin(&sensorData, &logging, &mcp0);
#ifdef USE_WATERLEVEL_SENSOR
#ifdef USE_MPXV5004
  waterLevelSensor.begin(&sensorData, &logging);
#elif defined (USE_FLOATSWITCHES)
#if defined(FLOATSWITCH_HIGH_MCP17_PIN) || defined(FLOATSWITCH_MEDIUM_MCP17_PIN) || defined(FLOATSWITCH_LOW_MCP17_PIN)
  waterLevelSensor.begin(&sensorData, &logging, &mcp0);
#else
  waterLevelSensor.begin(&sensorData, &logging);
#endif                                                      // Float switches pin type. 
#endif                                                      // USE_FLOATSWITCHES
#endif                                                      // USE_WATERLEVEL_SENSOR
#ifdef USE_DS18B20
  waterTempSensor.begin(&sensorData, &logging, &ds18b20, &oneWire);
#elif defined(USE_ISOLATED_SENSOR_BOARD)
  waterTempSensor.begin(&sensorData, &logging);
#endif
#endif
}

void readSensors() {
  for (uint8_t i = 0; i < nSensors; i++) {
    sensors[i]->readSensor();
    yield();
  }

  // Read the status of the flow sensors.
  uint8_t flowSensorPins = mcp1.readGPIO(0);                // 0 = GPIOA, 1 = GPIOB.
  if (flowSensorPins != oldFlowSensorPins) {                // One or more sensors changed status.
    for (uint8_t i = 0; i < TRAYS; i++) {
      if (bitRead(flowSensorPins, FLOW_PIN[i]) ^ bitRead(oldFlowSensorPins, FLOW_PIN[i])) {
        waterFlowSensorTicks[i]++;                          // Count the pulse of that flow sensor.
      }
    }
    oldFlowSensorPins = flowSensorPins;
  }
  trayPresence = ~mcp1.readGPIO(1) << 8;                    // Where the trays are connected. Invert (sensors are active low), then left shift by 8 as it's bank 1 (pins 8-15).


  trayPresence = 0xFF00;


#ifdef USE_ISOLATED_SENSOR_BOARD
  isolatedSensorBoard.readSensor();
#endif
}
