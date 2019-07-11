/***********************************************************************************************************
   setup()
*/
#ifdef USE_24LC256_EEPROM
E24LC256 EXTERNAL_EEPROM(0x50);
#endif

#define USE_SERIAL

void setup() {
#ifdef USE_DS1603L
  Serial.begin(9600);                                       // DS1603L sensor transmits its data at 9600 bps.
#elif defined(USE_SERIAL)
  Serial.begin(115200);
#endif
  MCP_init();                                               // Set up the port extenders.

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
  os_timer_setfn(&blinkTimer, blinkTimerCallback, NULL);
  os_timer_arm(&blinkTimer, 500, true);
#endif

#ifdef USE_24LC256_EEPROM
  sensorData.EEPROM = &EXTERNAL_EEPROM;
#endif

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V2_H
  mcp0.digitalWrite(AUX1_MCP17_PIN, HIGH);                  // Reset the port extenders.
  delay(500);
  mcp0.digitalWrite(AUX1_MCP17_PIN, LOW);
#endif

  core.begin(&sensorData);
  logging.begin(&sensorData);

  /***********************************************************************************************************
     WiFi related setup and configurations.
  */
#ifdef OPTION1
  WiFi.mode(WIFI_AP);                                       // Act as access point.
  WiFi.softAP(ap_ssid, ap_password);                        // Set up the access point (probably best to do this BEFORE trying to make a connection as station to prevent channel hopping issues).
  delay(100);                                               // Wait for AP to start up before configuring it.
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

#elif defined(OPTION2)
  WiFi.disconnect(true);                                    // Explicit disconnect of the station.
  WiFi.setAutoConnect(false);                               // Don't try to autoconnect WiFi as station. We only try to connect upon startup.
  WiFi.mode(WIFI_AP_STA);                                   // Act as station and access point.
  wifiMulti.addAP("Squirrel", "AcornsAreYummie");           // Home network.
  wifiMulti.addAP("Bustling City Tours", "bustlingcity");   // Phone network.
#ifdef USE_SERIAL
  Serial.print(F("Connecting to WiFi network..."));
#endif
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
#ifdef USE_SERIAL
    Serial.print(++i);
    Serial.print(' ');
#endif
    if (i > 30) {
#ifdef USE_SERIAL
      Serial.println();
      Serial.println(F("No network found - continuing without. No autoreconnect will take place."));
#endif
      WiFi.disconnect(true);                                // Explicit disconnect of the station.
      break;
    }
  }
  sprintf_P(buff, PSTR("WiFi connected to: %s"), WiFi.SSID().c_str());
  logging.writeInfo(buff);
  sprintf_P(buff, PSTR("IP address: %s"), WiFi.localIP().toString().c_str());
  logging.writeInfo(buff);

  WiFi.softAP(ap_ssid, ap_password);                        // Set up the access point.
  delay(100);                                               // Wait for AP to start up before configuring it.
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

#elif defined(OPTION3)
  WiFi.disconnect(true);                                    // Explicit disconnect of the station.
  WiFi.mode(WIFI_AP_STA);                                   // Act as station and access point.
  wifiMulti.addAP("Squirrel", "AcornsAreYummie");           // Home network.
  wifiMulti.addAP("Bustling City Tours", "bustlingcity");   // Phone network.
#ifdef USE_SERIAL
  Serial.print(F("Connecting to WiFi network..."));
#endif
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
#ifdef USE_SERIAL
    Serial.print(++i);
    Serial.print(' ');
#endif
    if (i > 30) {
#ifdef USE_SERIAL
      Serial.println();
      Serial.println(F("No network found - continuing without. System will continue to try to find a network."));
#endif
      WiFi.disconnect(true);                                // Explicit disconnect of the station.
      break;
    }
  }
  sprintf_P(buff, PSTR("WiFi connected to: %s"), WiFi.SSID().c_str());
  logging.writeInfo(buff);
  sprintf_P(buff, PSTR("IP address: %s"), WiFi.localIP().toString().c_str());
  logging.writeInfo(buff);

  WiFi.softAP(ap_ssid, ap_password);                        // Set up the access point.
  delay(100);                                               // Wait for AP to start up before configuring it.
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

#elif defined(OPTION4) || defined(OPTION5)
  WiFi.disconnect(true);                                    // Explicit disconnect of the station.

#ifdef OPTION4
  WiFi.setAutoConnect(false);                               // Don't try to autoconnect WiFi as station. We only try to connect upon startup.
#endif

  WiFi.mode(WIFI_AP_STA);                                   // Act as station and access point.
  WiFi.begin("Bustling City Tours", "bustlingcity");        // Phone network.
#ifdef USE_SERIAL
  Serial.print(F("Connecting to WiFi network..."));
#endif
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
#ifdef USE_SERIAL
    Serial.print(++i);
    Serial.print(' ');
#endif
    if (i > 30) {
#ifdef USE_SERIAL
      Serial.println();
      Serial.println(F("No network found - continuing without. No autoreconnect will take place."));
#endif
      WiFi.disconnect(true);                                // Explicit disconnect of the station.
      break;
    }
  }
  sprintf_P(buff, PSTR("WiFi connected to: %s"), WiFi.SSID().c_str());
  logging.writeInfo(buff);
  sprintf_P(buff, PSTR("IP address: %s"), WiFi.localIP().toString().c_str());
  logging.writeInfo(buff);

  WiFi.softAP(ap_ssid, ap_password);                        // Set up the access point.
  delay(100);                                               // Wait for AP to start up before configuring it.
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);


#endif

  // This hopefully solves connection problems at K11/Rosewood.
  //  WiFi.disconnect(true);                                    // Explicit disconnect of the station.
  //  ESP.eraseConfig();                                        // Clear any stored WiFi configurations.
  //  WiFi.setOutputPower(20.5);                                // Increase the output power (range: 0-20.5).
  //  WiFi.setPhyMode((WiFiPhyMode_t)PHY_MODE_11G);             // Make sure we're not in 802.11n mode, in which access point won't work (well).
  //  WiFi.setAutoConnect(false);                               // Don't try to autoconnect WiFi as station. We only try to connect upon startup.
  //  WiFi.mode(WIFI_AP_STA);                                   // Act as station and access point.

  //    // Network details.
  //    wifiMulti.addAP("Squirrel", "AcornsAreYummie");
  //    wifiMulti.addAP("Bustling City Tours", "bustlingcity");
  //
  //
  //  #ifdef USE_SERIAL
  //    Serial.print(F("Connecting to WiFi network..."));
  //  #endif
  //    int i = 0;
  //    while (wifiMulti.run() != WL_CONNECTED) {
  //      delay(1000);
  //  #ifdef USE_SERIAL
  //      Serial.print(++i);
  //      Serial.print(' ');
  //  #endif
  //      if (i > 12) {
  //  #ifdef USE_SERIAL
  //        Serial.println();
  //        Serial.println(F("No network found - continuing without. The system will connect on itself later when a network becomes available."));
  //  #endif
  //        break;
  //      }
  //    }
  //    sprintf_P(buff, PSTR("WiFi connected to: %s"), WiFi.SSID().c_str());
  //    logging.writeInfo(buff);
  //    sprintf_P(buff, PSTR("IP address: %s"), WiFi.localIP().toString().c_str());
  //    logging.writeInfo(buff);

  //  WiFi.softAP(ap_ssid, ap_password);                        // Set up the access point (probably best to do this BEFORE trying to make a connection as station to prevent channel hopping issues).
  //  //  WiFi.softAP(ssid, password, channel, hidden, max_connection)
  //  //  WiFi.softAP(ap_ssid, ap_password, channel, false, 4)      // channel: 1-13, default: 1.
  //  delay(100);                                               // Wait for AP to start up before configuring it.
  //  IPAddress local_IP(192, 168, 4, 1);
  //  IPAddress gateway(192, 168, 4, 1);
  //  IPAddress subnet(255, 255, 255, 0);
  //  WiFi.softAPConfig(local_IP, gateway, subnet);

  /***********************************************************************************************************

  */
#ifdef USE_OTA
  /* BEGIN OTA (part 2 of 3) */
  ArduinoOTA.setHostname(local_name);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.onStart([]() {
#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
    os_timer_arm(&blinkTimer, 200, true);
#endif
    Serial.println(F("Start"));
  });
  ArduinoOTA.onEnd([]() {
#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
    os_timer_disarm(&blinkTimer);
    writeLed(LOW);
#endif
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf_P(PSTR("Progress: % u % % \r"), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf_P(PSTR("Error[ % u]: "), error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();
  Serial.println(F("OTA ready."));
  /* END OTA (part 2 of 3) */

  // Recovery option: in case of any crashes later in the setup, this allows for easy recovery by uploading a new
  // sketch OTA instead of having to fall back on Serial right away.
  // Blink WIFILED fast to indicate we're at this stage.
#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
  os_timer_arm(&blinkTimer, 200, true);
#endif
  Serial.println(F("Starting 5 - second OTA recovery window."));
  uint32_t startTime = millis();
  while (millis() - startTime < 5000) {
    ArduinoOTA.handle();
    yield();
  }
  Serial.println(F("Recovery window closed; continuing normal setup."));
#endif

  fertiliser.begin(&sensorData, &logging, &mcp0);           // Fertiliser pumps.
  pHMinus.begin(&sensorData, &logging, &mcp0);              // pH minus pump.
#ifdef USE_WATERLEVEL_SENSOR
  reservoir.begin(&sensorData, &logging, &mcp0, &waterLevelSensor); // Solenoid valve.
  drainage.begin(&sensorData, &logging, &mcp0, &waterLevelSensor); // Drainage pump & reservoir cleaning.
#else
  reservoir.begin(&sensorData, &logging, &mcp0);            // Solenoid valve.
  drainage.begin(&sensorData, &logging, &mcp0);             // Drainage pump & reservoir cleaning.
#endif

  // Set up the EEPROM memory (4096 bytes).
#ifndef USE_24LC256_EEPROM
  EEPROM.begin(EEPROM_SIZE);
#endif

  initSensors();

  // The network functions.
  network.begin(&sensorData, &logging, &server);
  initWebsite();

  // Read all the crop names and IDs.
  getCropIds();

  // Read the latest trayInfo from EEPROM.
#ifdef USE_24LC256_EEPROM
  sensorData.EEPROM->get(TRAYINFO_EEPROM, trayInfo);
  sensorData.EEPROM->get(FLOWSENSOR_COUNT_EEPROM, flowSensorCount);
#else
  EEPROM.get(TRAYINFO_EEPROM, trayInfo);
  EEPROM.get(FLOWSENSOR_COUNT_EEPROM, flowSensorCount);
#endif

  // Check wether we have valid data in the tray info - if not reset this tray.
  for (uint8_t tray = 0; tray < TRAYS; tray++) {
    if (isValidFrequency(trayInfo[tray].wateringFrequency) == false) {
      trayInfo[tray].programState = PROGRAM_UNSET;
      trayInfo[tray].cropId = 0;
      trayInfo[tray].startTime = 0;
#ifdef USE_GROWLIGHT
      trayInfo[tray].darkDays = 0;
#endif
      trayInfo[tray].totalDays = 0;
      trayInfo[tray].wateringFrequency = 0;
    }
    wateringTime[tray] = 0;
    //    wateringState[tray] = DRAINING;
  }
  //  if (!MDNS.begin(local_name)) {                            // Start the mDNS responder for local_name.local
  //    Serial.println("Error setting up MDNS responder!");
  //  }
  //  else {
  //    Serial.println("mDNS responder started");
  //  }

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
  // Stop blinking the WiFi LED - by the time we're here, WiFi is connected and we're ready to go.
  os_timer_disarm(&blinkTimer);
  mcp0.digitalWrite(WIFI_LED_PIN, HIGH);
#endif
}

void MCP_init() {
  mcp0.begin(0);                                            // The mcp23017 port expanders.
  mcp1.begin(1);

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V2_H
  // Force a reset upon the mcp port extenders.
  pinMode(MCP_RESET, OUTPUT);
  digitalWrite(MCP_RESET, HIGH);                            // Force reset.
  delay(10);                                                // Give some time for a proper reset.

  // Enable the mcp port extenders - bring the RESET pin high by setting the output LOW.
  digitalWrite(MCP_RESET, LOW);                             // Bring the RESET pin of the MCP port extenders HIGH.
  delay(10);                                                // Allow the ICs to start up.
#endif

#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
  mcp0.pinMode(WIFI_LED_PIN, OUTPUT);                       // WiFi indicator LED.
  os_timer_setfn(&blinkTimer, blinkTimerCallback, NULL);
  os_timer_arm(&blinkTimer, 500, true);
#endif

  for (uint8_t tray = 0; tray < TRAYS; tray++ ) {
    mcp1.pinMode(FLOW_PIN[tray], INPUT);                    // The flow sensors.
    mcp1.pullUp(FLOW_PIN[tray], true);                      // Hall effect sensor: driven high/low.
    mcp1.pinMode(TRAY_PIN[tray], INPUT);                    // The tray pins.
    mcp1.pullUp(TRAY_PIN[tray], true);                      // Open collector output.
    mcp0.pinMode(PUMP_PIN[tray], OUTPUT);                   // The water pumps.
    mcp0.digitalWrite(PUMP_PIN[tray], LOW);                 // Make sure those pumps are off.
  }
  oldFlowSensorPins = mcp1.readGPIO(0);
#ifdef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
  mcp0.pinMode(DOOR_PIN, INPUT);                            // The door open/close sensor.
  mcp0.pullUp(DOOR_PIN, true);
#endif
  mcp0.pinMode(LEVEL_LIMIT_MCP17_PIN, INPUT);               // Water level limit switch.
  mcp0.pullUp(LEVEL_LIMIT_MCP17_PIN, false);                // Don't use the pull-up here!
  mcp0.pinMode(AUX1_MCP17_PIN, OUTPUT);
}
