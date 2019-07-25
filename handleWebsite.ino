/***********************************************************************************************************
   The http request handlers.
*/
/***********************************************************************************************************
  Root: return the main page, showing the status of the system.
*/
void initWebsite() {

  // Set up the http request handlers.
  server.on("/", handleRoot);
  server.on("/api", handleAPI);
  //  server.on("/hydrodata.json", handleJSON);
  server.on("/settings", handleSettings);
  server.on("/set_settings", handleSetSettings);
  server.on("/calibrate_ec", handleCalibrateEC);
  server.on("/calibrate_ec_action", handleCalibrateECAction);
  server.on("/calibrate_ph", handleCalibratepH);
  server.on("/calibrate_ph_action", handleCalibratepHAction);
  server.on("/calibrate_flowsensors", handleCalibrateFlowsensors);
  server.on("/calibrate_flowsensors_action_start", handleCalibrateFlowsensorsActionStart);
  server.on("/calibrate_flowsensors_action_stop", handleCalibrateFlowsensorsActionStop);
  server.on("/measure_pump_a_speed", handlePumpASpeed);
  server.on("/measure_pump_b_speed", handlePumpBSpeed);
  server.on("/measure_pump_phminus_speed", handlePumppHMinusSpeed);
  server.on("/drain_start", handleDrainStart);
  server.on("/drain_stop", handleDrainStop);
#ifdef USE_WATERLEVEL_SENSOR
#ifdef USE_MPXV5004
  server.on("/zero_reservoir_level", handleZeroReservoirLevel);
  server.on("/max_reservoir_level", handleMaxReservoirLevel);
#endif
#endif
  server.onNotFound(handleNotFound);
  server.begin();
}

void handleRoot() {

  // If we have arguments, handle that here.
  if (server.args() > 0) {
    handleCropProgramRequest();
  }

  network.htmlResponse(&server);
  network.htmlPageHeader(&server, false);
  server.sendContent_P(PSTR("<p>Current time: "));
  datetime(buff);
  server.sendContent(buff);
  server.sendContent_P(PSTR("</p>\n\
    <p></p>\n\
    <table>\n\
      <tr>\n\
        <th>Parameter</th>\n\
        <th>Value</th>\n\
      </tr>"));
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    sensors[i]->dataHtml(&server);
  }
  server.sendContent_P(PSTR("\n\
    </table>\n"));

  // The details of each individual tray, placed into an html table.
  for (int8_t i = 0; i < TRAYS; i++) {
    int32_t programTime = 0;
    if (trayInfo[i].totalDays > 0) {
      programTime = (trayInfo[i].totalDays * 24 - 4) * 3600; // The duration of the program of this tray in seconds.
    }
    uint32_t programTimeLeft = 0;
    uint32_t programFinishTime = 0;
    if (trayInfo[i].startTime > 0) {                        // Program has a start time, so we can calculate a finish time.
      programFinishTime = trayInfo[i].startTime + programTime;
      if (timeStatus() != timeNotSet) {                     // Program has a start time, and we have the current time, so we can calculate the actual time left.
        programTimeLeft = programFinishTime - now();
      }
    }
    else if (trayInfo[i].programState != PROGRAM_UNSET) {
      programTimeLeft = programTime;                        // Program has no start time, so time left is total program time.
      if (timeStatus() != timeNotSet) {
        programFinishTime = now() + programTime;            // If we have the time we calculate the projected finish time if started now.
      }
    }

    // The table in html code.
    server.sendContent_P(PSTR("\
    <form action=\"/\" method=\"POST\">\n\
      <table>\n\
        <tr>\n\
          <td>\n\
            <input type=\"hidden\" name=\"tray\" value=\""));
    server.sendContent(itoa(i, buff, 10));                  // The tray number.
    server.sendContent_P(PSTR("\">\n\
            Tray "));
    server.sendContent(itoa(i + 1, buff, 10));              // Add one for humans, who start counting at 1.
    server.sendContent_P(PSTR("<br>\n"));
    if (bitRead(trayPresence, TRAY_PIN[i])) {               // Show whether the tray is present or not.
      server.sendContent_P(PSTR("\
            Tray present."));
    }
    else {
      server.sendContent_P(PSTR("\
            Tray not present."));
    }
    server.sendContent_P(PSTR("<br>\n"));
    switch (trayInfo[i].programState) {
      case PROGRAM_UNSET:
        server.sendContent_P(PSTR("\
            No program set."));
        break;

      case PROGRAM_SET:
        server.sendContent_P(PSTR("\
            Crop has been set."));
        break;

      case PROGRAM_START:
        server.sendContent_P(PSTR("\
            Program is starting."));
        break;

      case PROGRAM_START_WATERING:
        server.sendContent_P(PSTR("\
            Program start: initial watering."));
        break;

      case PROGRAM_RUNNING:
        server.sendContent_P(PSTR("\
            Program in progress."));
        break;

      case PROGRAM_COMPLETE:
        server.sendContent_P(PSTR("\
            Program complete."));
        break;

      case PROGRAM_PAUSED:
      case PROGRAM_START_PAUSED:
        server.sendContent_P(PSTR("\
            Program on hold - no lights, no watering."));
        break;

      case PROGRAM_ERROR:
        server.sendContent_P(PSTR("\
            Program on hold - error detected."));
        break;
    }

    server.sendContent_P(PSTR("<br>\n"));
    switch (wateringState[i]) {
      case WATERING_NEEDED:
        server.sendContent_P(PSTR("\
            Watering: pending watering."));
        break;

      case WATERING:
        server.sendContent_P(PSTR("\
            Watering: "));
        server.sendContent(itoa(waterFlowSensorTicks[i], buff, 10));
        server.sendContent_P(PSTR(" ticks."));
        break;

      case DRAINING:
        server.sendContent_P(PSTR("\
            Watering: tray draining."));
        break;

      default:
        server.sendContent_P(PSTR("\
            Watering: idle."));
        break;
    }

    server.sendContent_P(PSTR("\n\
          </td>\n\
          <td>"));
    cropProgramHtml(i);                                     // The crop selection or program details, as appropriate for the program status.
    server.sendContent_P(PSTR("\n\
          </td>\n\
          <td>\n"));
    buttonHtml(i);                                          // One or more buttons to start/pause/reset/resume programs.
    server.sendContent_P(PSTR("\
          </td>\n\
        </tr>\n\
        <tr>\n"));
    if (trayInfo[i].startTime > 0) {
      server.sendContent_P(PSTR("\
          <td>Started: "));
      datetime(buff, trayInfo[i].startTime);                // Timestamp: when the program started.
      server.sendContent(buff);
      server.sendContent_P(PSTR("</td>\n"));
    }
    else {
      server.sendContent_P(PSTR("\
          <td></td>\n"));
    }
    if (trayInfo[i].totalDays > 0) {
      server.sendContent_P(PSTR("\
          <td colspan = \"2\">Harvest: "));
      datetime(buff, programFinishTime);                    // Timestamp: when the crop is ready for harvest.
      server.sendContent(buff);
      server.sendContent_P(PSTR("</td>\n"));
    }
    else {
      server.sendContent_P(PSTR("\
          <td colspan = \"2\"></td>\n"));
    }
    server.sendContent_P(PSTR("\
        </tr><tr>\n"));
    if (programTimeLeft > 0) {
      server.sendContent_P(PSTR("\
          <td>Time to harvest: "));
      daysHours(buff, programTimeLeft);                     // The time from now until harvest, as days/hours or hours/minutes or minutes.
      server.sendContent(buff);
      server.sendContent_P(PSTR("</td>\n"));
    }
    else {
      server.sendContent_P(PSTR("\
          <td></td>\n"));
    }
    server.sendContent_P(PSTR("\
          <td colspan = \"2\"></td>\n\
        </tr>\n\
      </table>\n\
    </form>\n"));
  }
  server.sendContent_P(PSTR("\
    <p><a href=\"/settings\">Settings</a></p>\
    <p><a href=\"/calibrate_flowsensors\">Calibrate flow sensors</a></p>\
  </body></html>"));
  network.htmlPageFooter(&server);
}

void handleCropProgramRequest() {
  uint8_t nArgs = server.args();                            // the number of arguments present. All arguments are expected to be key/value pairs.
  String command = "";
  String keys[nArgs];
  uint32_t values[nArgs];

  // Parse the arguments, and do a first sanity check on them.
  // All values except the command are numerical, handle as such and command separately.
  for (uint8_t i = 0; i < nArgs; i++) {
    values[i] = 0;
    keys[i] = server.argName(i);

    // All arguments are an integer value, except the command which is handled separately.
    if (keys[i] != F("command")) {
      values[i] = server.arg(i).toInt();
    }
    else {
      command = server.arg(i);                              // The command of this query.
    }
  }
  if (command != "") {                                      // Must have a command for a valid query!
    uint8_t tray;
    uint8_t id;
    bool argsValid = true;

    // Most commands have a tray parameter; take care of that one here.
    bool haveTray = false;
    for (uint8_t i = 0; i < nArgs; i++) {                   // Search for the tray number.
      if (keys[i] == F("tray")) {
        if (values[i] >= TRAYS) {                           // Invalid tray number given, invalidate arguments.
          argsValid = false;
          break;
        }
        tray = values[i];
        haveTray = true;
        break;
      }
    }
    if (argsValid) {

      // command set_crop
      if (command == F("set_crop") &&
          haveTray &&
          trayInfo[tray].programState == PROGRAM_UNSET) {   // Set a new crop for a specific tray.
        getProgramData(keys, values, nArgs, &id, tray, &argsValid);
        if (argsValid) {
          trayInfo[tray].cropId = id;
          trayInfo[tray].programState = PROGRAM_SET;
          trayInfoChanged = true;
        }
      }
      // Command start.
      if (command == F("start") &&
          haveTray &&
          (trayInfo[tray].programState == PROGRAM_UNSET ||
           trayInfo[tray].programState == PROGRAM_SET ||
           trayInfo[tray].programState == PROGRAM_COMPLETE)) {
        Serial.println(F("Got command start."));
        if (trayInfo[tray].cropId == 255 ||                 // Custom program comes with extra parameters for the crop program.
            trayInfo[tray].programState == PROGRAM_UNSET) { // If tray is still unset: expect to get the cropId in the arguments (it comes from the app like this).
          getProgramData(keys, values, nArgs, &id, tray, &argsValid);
          Serial.print(F("Got crop ID: "));
          Serial.println(id);
          trayInfo[tray].cropId = id;
        }
        if (argsValid) {
          trayInfo[tray].startTime = now();
          trayInfo[tray].programState = PROGRAM_START;
          trayInfoChanged = true;
        }
      }

      // Command resume.
      if (command == F("resume")) {
        switch (trayInfo[tray].programState) {
          case PROGRAM_PAUSED:
          case PROGRAM_ERROR:
            trayInfo[tray].programState = PROGRAM_RUNNING;
            trayInfoChanged = true;
            break;

          case PROGRAM_START_PAUSED:
            trayInfo[tray].programState = PROGRAM_START;
            trayInfoChanged = true;
            break;
        }
      }

      // Command pause.
      if (command == F("pause")) {
        switch (trayInfo[tray].programState) {
          case PROGRAM_RUNNING:
            trayInfo[tray].programState = PROGRAM_PAUSED;
            trayInfoChanged = true;
            break;

          case PROGRAM_START:
          case PROGRAM_START_WATERING:
            trayInfo[tray].programState = PROGRAM_START_PAUSED;
            trayInfoChanged = true;
            break;
        }
      }

      // Command reset
      if (command == F("reset")) {
        trayInfo[tray].programState = PROGRAM_UNSET;
        trayInfo[tray].cropId = 0;
        trayInfo[tray].startTime = 0;
#ifdef USE_GROWLIGHT
        trayInfo[tray].darkDays = 0;
#endif
        trayInfo[tray].totalDays = 0;
        trayInfo[tray].wateringFrequency = 0;
        trayInfoChanged = true;
        switch (wateringState[tray]) {
          case WATERING_NEEDED:
            wateringState[tray] = WATERING_IDLE;
            break;

          case WATERING:
            wateringState[tray] = DRAINING;
            wateringTime[tray] = millis();
            break;
        }
      }
    }
  }
}

/***********************************************************************************************************
  Any unknown request is handled here.
*/
void handleNotFound() {
  server.send(200, F("text/plain"), F("Unknown request received."));
}

/***********************************************************************************************************

*/
String validateDate(String date) {

  //TODO implement this.

  return date;
}

const char PROGMEM resetButton[] = "\
          <button type=\"submit\" name=\"command\" value=\"reset\">Reset</button>\n\
          &nbsp;\n";

//********************************************************************************************************************
// Create html button for a specific tray, based on current status and available crops.
void buttonHtml(uint8_t tray) {
  switch (trayInfo[tray].programState) {
    case PROGRAM_UNSET:
      server.sendContent_P(PSTR("\n\
          <button type=\"submit\" name=\"command\" value=\"set_crop\">Set crop</button>\n\
          &nbsp;\n"));
      break;
    case PROGRAM_SET:
      server.sendContent_P(PSTR("\n\
          <button type=\"submit\" name=\"command\" value=\"start\">Start</button>\n\
          &nbsp;\n"));
      server.sendContent_P(resetButton);
      break;
    case PROGRAM_RUNNING:
    case PROGRAM_START:
    case PROGRAM_START_WATERING:
      server.sendContent_P(PSTR("\n\
          <button type=\"submit\" name=\"command\" value=\"pause\">Pause</button>\n\
          &nbsp;\n"));
      break;
    case PROGRAM_PAUSED:
    case PROGRAM_START_PAUSED:
    case PROGRAM_ERROR:
      server.sendContent_P(PSTR("\n\
          <button type=\"submit\" name=\"command\" value=\"resume\">Resume</button>\n\
          &nbsp;\n"));
      server.sendContent_P(resetButton);
      break;
    case PROGRAM_COMPLETE:
      server.sendContent_P(PSTR("\n\
          <button type=\"submit\" name=\"command\" value=\"start\">Start again</button>\n\
          &nbsp;\n"));
      server.sendContent_P(resetButton);
      break;

    default:
      server.sendContent_P(resetButton);
      break;
  }
}

#ifdef USE_WATERLEVEL_SENSOR
#ifdef USE_MPXV5004
void handleZeroReservoirLevel() {
  waterLevelSensor.setZero();
  handleSettings();
}

void handleMaxReservoirLevel() {
  waterLevelSensor.setMax();
  handleSettings();
}
#endif
#endif
