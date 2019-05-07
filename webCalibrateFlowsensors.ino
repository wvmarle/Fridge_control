void handleCalibrateFlowsensors() {
  network.htmlResponse(&server);
  network.htmlPageHeader(&server, false);
  server.sendContent_P(PSTR("\
  <h1>Flow sensors calibration.</h1>\n\
  <p></p>\n\
  <table>\n\
    <tr>\n\
      <th>Tray</th>\n\
      <th>Action</th>\n\
      <th>Measured pulses</th>\n\
      <th></th>\n\
    </tr>"));
  for (uint8_t i = 0; i < TRAYS; i++) {
    server.sendContent_P(PSTR("\n\
    <tr>\n\
      <td>"));
    server.sendContent(itoa(i + 1, buff, 10));
    server.sendContent_P(PSTR("</td>"));

    bool canCalibrate = false;
    if (bitRead(sensorData.systemStatus, STATUS_RESERVOIR_LEVEL_LOW) == false  // There's enough water in the reservoir
        && bitRead(trayPresence, TRAY_PIN[i])               // and the tray is present
        && (trayInfo[i].programState == PROGRAM_UNSET
            || trayInfo[i].programState  == PROGRAM_SET)    // and there's no program active,
        && wateringState[i] == WATERING_IDLE) {             // and this tray is not active in watering or draining,
      canCalibrate = true;                                  // We can calibrate this tray.
    }
    if (canCalibrate) {
      server.sendContent_P(PSTR("\n\
      <td>\n\
        <form action = \"/calibrate_flowsensors_action_start\">\n\
          <input type=\"hidden\" name=\"flow_sensor\" value = \""));
      server.sendContent(itoa(i, buff, 10));                // The start button for this tray.
      server.sendContent_P(PSTR("\">\n\
          <input type=\"submit\" value=\"Start\">\n\
        </form>\n\
      </td>\n\
      <td>Pulses: "));
      server.sendContent(itoa(flowSensorCount[i], buff, 10)); // The current calibration number.
      server.sendContent_P(PSTR(".</td>\n\
      <td></td>"));
    }
    else if (wateringState[i] == CALIBRATING) {             // Calibration in progress.
      server.sendContent_P(PSTR("\n\
      <td>\n\
        <form action = \"/calibrate_flowsensors_action_stop\">\n\
          <input type=\"hidden\" name=\"flow_sensor\" value = \""));
      server.sendContent(itoa(i, buff, 10));                // The stop button for this tray.
      server.sendContent_P(PSTR("\">\n\
          <input type=\"submit\" value=\"Stop\">\n\
        </form>\n\
      </td>\n\
      <td>Calibration running.</td>\n\
      <td></td>"));                                         // Show pump is running.
    }
    else {                                                  // Can't calibrate: greyed-out button.
      server.sendContent_P(PSTR("\n\
      <td>\n\
        <form>\n\
          <input type=\"submit\" value=\"Start\" disabled>\n\
        </form>\n\
      </td>\n\
      <td>Pulses: "));
      server.sendContent(itoa(flowSensorCount[i], buff, 10)); // The current calibration number.
      server.sendContent_P(PSTR(".</td>\n\
      <td>"));
      if (wateringState[i] == DRAINING) {                   // Watering state: draining; can't start calibration now.
        server.sendContent_P(PSTR("\
        Tray draining. Please wait."));
      }
      else if (bitRead(trayPresence, TRAY_PIN[i]) == false) { // Tray not present.
        server.sendContent_P(PSTR("\
        Tray not present."));
      }
      else if (bitRead(sensorData.systemStatus, STATUS_RESERVOIR_LEVEL_LOW)) { // There's not enough water in the reservoir.
        server.sendContent_P(PSTR("\
        Not enough water in the reservoir."));
      }
      else if (!(trayInfo[i].programState == PROGRAM_UNSET
                 || trayInfo[i].programState  == PROGRAM_SET)) { // There's a program active.
        server.sendContent_P(PSTR("\
        Tray has a crop program running."));
      }
      server.sendContent_P(PSTR("</td>"));
    }
    server.sendContent_P(PSTR("\n\
    </tr>"));
  }
  server.sendContent_P(PSTR("\n\
  </table>"));
  network.htmlPageFooter(&server);
}

void handleCalibrateFlowsensorsActionStart() {
  uint8_t nArgs = server.args();                            // The number of arguments present. All arguments are expected to be key/value pairs.
  for (uint8_t i = 0; i < nArgs; i++) {                     // Read all the arguments that are present.
    if (server.argName(i) == F("flow_sensor")) {
      if (core.isNumeric(server.arg(i))) {
        uint8_t tray = server.arg(i).toInt();
        if (tray < TRAYS) {
          if (wateringState[tray] == WATERING_IDLE &&       // Make sure the tray isn't watering or still draining, and
              bitRead(trayPresence, TRAY_PIN[tray])) {      // that the tray is even present.
            wateringState[tray] = CALIBRATING;
            waterFlowSensorTicks[tray] = 0;
            mcp0.digitalWrite(PUMP_PIN[tray], HIGH);
            bitSet(sensorData.systemStatus, STATUS_WATERING);
          }
        }
      }
    }
  }
  handleCalibrateFlowsensors();
}

void handleCalibrateFlowsensorsActionStop() {
  uint8_t nArgs = server.args();                            // the number of arguments present. All arguments are expected to be key/value pairs.
  for (uint8_t i = 0; i < nArgs; i++) {                     // Read all the arguments that are present.
    if (server.argName(i) == F("flow_sensor")) {
      if (core.isNumeric(server.arg(i))) {
        uint8_t tray = server.arg(i).toInt();
        if (tray < TRAYS) {
          if (wateringState[tray] == CALIBRATING) {
            mcp0.digitalWrite(PUMP_PIN[tray], LOW);
            wateringState[tray] = DRAINING;
            wateringTime[tray] = millis();
            flowSensorCount[tray] = waterFlowSensorTicks[tray];
#ifdef USE_24LC256_EEPROM
            sensorData.EEPROM->put(FLOWSENSOR_COUNT_EEPROM, flowSensorCount);
#else
            EEPROM.put(FLOWSENSOR_COUNT_EEPROM, flowSensorCount);
            EEPROM.commit();
#endif
          }
        }
      }
    }
  }
  handleCalibrateFlowsensors();
}

/*
   Send JSON object with tray calibration info {status, count}.

   status:
      1 available for calibration.
      2 calibration in progress.
      3 tray not present.
      4 tray has program running.
      5 tray is draining.
      6 reservoir level too low.
*/
void getFlowsensorCalibration() {
  sendJsonHeader();
  server.sendContent_P(PSTR("{\"flowsensors\":\n"
                            "  {\n"));
  bool isFirst = true;
  for (uint8_t tray = 0; tray < TRAYS; tray++) {
    if (isFirst) {
      isFirst = false;
    }
    else {
      server.sendContent_P(PSTR(",\n"));
    }
    server.sendContent_P(PSTR("    \""));
    server.sendContent(itoa(tray, buff, 10));
    server.sendContent_P(PSTR("\":[\""));
    uint8_t calibrationStatus = 1;                          // 1: tray is free for calibration.
    if (wateringState[tray] == CALIBRATING) {
      calibrationStatus = 2;                                // 2: tray calibration in progress.
    }
    else if (bitRead(trayPresence, TRAY_PIN[tray]) == false) {
      calibrationStatus = 3;                                // 3: tray is not present.
    }
    else if (trayInfo[tray].programState != PROGRAM_UNSET
             && trayInfo[tray].programState != PROGRAM_SET) {
      calibrationStatus = 4;                                // 4: program in progress.
    }
    else if (wateringState[tray] == DRAINING) {
      calibrationStatus = 5;                                // 5: tray is draining.
    }
    else if (bitRead(sensorData.systemStatus, STATUS_RESERVOIR_LEVEL_LOW)) {
      calibrationStatus = 6;                                // 6: reservoir level is too low.
    }
    server.sendContent(itoa(calibrationStatus, buff, 10));
    server.sendContent_P(PSTR("\",\""));
    server.sendContent(itoa(flowSensorCount[tray], buff, 10));
    server.sendContent_P(PSTR("\"]"));
  }
  server.sendContent_P(PSTR("  }\n"
                            "}"));
}
