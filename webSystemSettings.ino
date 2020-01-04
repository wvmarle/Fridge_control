
/***********************************************************************************************************
  /settings: create a page with all the settings of the enabled peripherals.
*/
void handleSettings() {
  network.htmlResponse();
  network.htmlPageHeader(false);
  server.sendContent_P(PSTR("\
  <form action=\"/set_settings\" method=\"post\">\n\
    <table>\n"));
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    sensors[i]->settingsHtml(&server);
  }
  fertiliser.settingsHtml(&server);
  pHMinus.settingsHtml(&server);
  reservoir.settingsHtml(&server);
  drainage.settingsHtml(&server);
  logging.settingsHtml(&server);
  network.settingsHtml(&server);
  parameters.settingsHtml(&server);
  server.sendContent_P(PSTR("\
    </table>\n\
    <input type=\"submit\" value=\"Save\">\n\
  </form>\n"));
  network.htmlPageFooter();
}

/***********************************************************************************************************
  /set_settings: receive a new set of settings, and apply them.
*/
void handleSetSettings() {
  setSettings();

  // Return to the settings page.
  network.redirectTo("/settings");
}

void setSettings() {

  // Call the updateSettings() function of all enabled sensors, where the relevant settings
  // can be handled.
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    sensors[i]->updateSettings(&server);
  }
  fertiliser.updateSettings(&server);
  pHMinus.updateSettings(&server);
  logging.updateSettings(&server);
  network.updateSettings(&server);
  parameters.updateSettings(&server);
  drainage.updateSettings(&server);
  reservoir.updateSettings(&server);
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(F("Argument: "));
    Serial.print(server.argName(i));
    Serial.print(F(", value: "));
    Serial.println(server.arg(i));
  }
}

/***********************************************************************************************************
  Measure the speed of fertiliser pump A.
*/
void handlePumpASpeed() {
  fertiliser.measurePumpA();
  network.redirectTo("/settings");
}

/***********************************************************************************************************
  Measure the speed of fertiliser pump B.
*/
void handlePumpBSpeed() {
  fertiliser.measurePumpB();
  network.redirectTo("/settings");
}

/***********************************************************************************************************
  Measure the speed of the pH-minus pump.
*/
void handlePumppHMinusSpeed() {
  pHMinus.measurePump();
  network.redirectTo("/settings");
}

/***********************************************************************************************************
  Start draining the reservoir through the reservoir drainage pump.
*/
void handleDrainStart() {
  drainage.drainStart();
  network.redirectTo("/settings");
}

/***********************************************************************************************************
  Stop draining the reservoir through the reservoir drainage pump.
*/
void handleDrainStop() {
  drainage.drainStop();
  network.redirectTo("/settings");
}
