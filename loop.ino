void loop() {
#ifdef USE_OTA
  /* BEGIN OTA (part 3 of 3) */
  ArduinoOTA.handle();
  /* END OTA (part 3 of 3) */
#endif

  // Check for incoming http connections.
  uint32_t startLoop = millis();
  server.handleClient();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  fertiliser.doFertiliser();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  pHMinus.dopH();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  reservoir.doReservoir();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  drainage.doDrainage();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  readSensors();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  handleTrays();
//  Serial.print(millis() - startLoop);
//  Serial.print(" ... ");
  logging.logData();
//  Serial.print(millis() - startLoop);
//  Serial.println();

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
