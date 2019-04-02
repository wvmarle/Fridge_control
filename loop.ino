void loop() {
#ifdef USE_OTA
  /* BEGIN OTA (part 3 of 3) */
  ArduinoOTA.handle();
  /* END OTA (part 3 of 3) */
#endif

  // Check for incoming http connections.
  server.handleClient();
  
  fertiliser.doFertiliser();
  pHMinus.dopH();
  reservoir.doReservoir();
  drainage.doDrainage();
  readSensors();
  handleTrays();
  logging.logData();

  // Every REFRESH_NTP seconds: update the time.
  if (millis() - lastNtpUpdateTime > REFRESH_NTP) {
    lastNtpUpdateTime += REFRESH_NTP;
    network.ntpUpdateInit();
    updatingTime = true;
  }
  if (updatingTime) updatingTime = network.ntpCheck();

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
}
