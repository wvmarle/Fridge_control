void loop() {
#ifdef USE_OTA
  /* BEGIN OTA (part 3 of 3) */
  ArduinoOTA.handle();
  /* END OTA (part 3 of 3) */
#endif

  // Check for incoming http connections.
  uint32_t startLoop = millis();
  server.handleClient();
  yield();
  fertiliser.doFertiliser();
  yield();
  pHMinus.dopH();
  yield();
  reservoir.doReservoir();
  yield();
  drainage.doDrainage();
  yield();
  readSensors();
  yield();
  handleTrays();
  yield();
  logging.logData();
  yield();

  //  // Every REFRESH_NTP seconds: update the time.
  //  if (millis() - lastNtpUpdateTime > REFRESH_NTP) {
  //    lastNtpUpdateTime += REFRESH_NTP;
  //    network.ntpUpdateInit();
  //    updatingTime = true;
  //  }
  //  if (updatingTime) updatingTime = network.ntpCheck();

  // Update the EEPROM.
  if (trayInfoChanged) {
#ifdef USE_24LC256_EEPROM
    sensorData.EEPROM->put(TRAYINFO_EEPROM, trayInfo);
#else
    EEPROM.put(TRAYINFO_EEPROM, trayInfo);
    EEPROM.commit();
#endif
    trayInfoChanged = false;
  }
#ifdef OPTION4
  static wifiConnected = true;
  if (WiFi.status() != WL_CONNECTED &&
      wifiConnected) {                                      // Connection lost.
    wifiConnected = false;
    WiFi.disconnect(true);                                  // Formally disconnect from network.
  }
#endif
}
