
/***********************************************************************************************************
  /api: this is communication from the app.
*/
void handleAPI() {
  uint8_t nArgs = server.args();                            // the number of arguments present. All arguments are expected to be key/value pairs.
  APIRequest = true;                                        // Flag that we're currently handling an API request, so handlers can select the appropriate return.
  if (nArgs > 0) {
    String request = server.arg(F("request"));

    Serial.print(F("Received API request with "));
    Serial.print(nArgs);
    Serial.print(F(" arguments, and request: "));
    Serial.println(request);
    uint32_t startMillis = millis();
    
    // getdata: return all available sensor data as JSON string.
    if (request == F("get_data")) {
      sendSystemData();
    }

    else if (request == F("get_crop_data")) {
      sendCropList();
    }

    // calibration: get calibration data of a specific sensor.
    // Expect one argument: sensor, which is the name of one the sensor of which the data is requested.
    else if (request == F("get_calibration")) {
      String sensor = server.arg(F("sensor"));
      if (sensor == F("ec")) {
        ECSensor.getCalibration(&server);
      }
      else if (sensor == F("ph")) {
        pHSensor.getCalibration(&server);
      }
      else {
        server.send(200, F("text/plain"), F("Calibration api request received with invalid sensor."));
      }
    }

    else if (request == F("do_calibration")) {
      String sensor = server.arg(F("sensor"));
      if (sensor == F("ec")) {
        ECSensor.doCalibration(&server);
        ECSensor.getCalibration(&server);
      }
      else if (sensor == F("ph")) {
        pHSensor.doCalibration(&server);
        pHSensor.getCalibration(&server);
      }
      else {
        server.send(200, F("text/plain"), F("Calibration api request received with invalid sensor."));
      }
    }

    else if (request == F("delete_calibration")) {
      String sensor = server.arg(F("sensor"));
      if (sensor == F("ec")) {
        ECSensor.deleteCalibration(&server);
        ECSensor.getCalibration(&server);
      }
      else if (sensor == F("ph")) {
        pHSensor.deleteCalibration(&server);
        pHSensor.getCalibration(&server);
      }
      else {
        server.send(200, F("text/plain"), F("Calibration api request received with invalid sensor."));
      }
    }

    else if (request == F("enable_calibration")) {
      String sensor = server.arg(F("sensor"));
      if (sensor == F("ec")) {
        ECSensor.enableCalibration(&server);
        ECSensor.getCalibration(&server);
      }
      else if (sensor == F("ph")) {
        pHSensor.enableCalibration(&server);
        pHSensor.getCalibration(&server);
      }
      else {
        server.send(200, F("text/plain"), F("Calibration api request received with invalid sensor."));
      }
    }

    // set_settings: set the various settings of a sensor.
    // Can be handled by the general function. Respond with the (updated) settings as JSON object.
    else if (request == F("set_settings")) {
      setSettings();
      sendSettingsJSON();
    }

    // get_settings: get the various settings of the system as JSON object.
    else if (request == F("get_settings")) {
      sendSettingsJSON();
    }

    // measure_pump_speed: run the indicated pump for 60 seconds, so the user can measure the flow
    // of this pump.
    else if (request == F("measure_pump_speed")) {
      String pump = server.arg(F("pump"));
      if (pump == F("pump_a")) {
        fertiliser.measurePumpA();
      }
      if (pump == F("pump_b")) {
        fertiliser.measurePumpB();
      }
      if (pump == F("pump_ph")) {
        pHMinus.measurePump();
      }
      sendSettingsJSON();
    }
#ifdef USE_WATERLEVEL_SENSOR
    else if (request == F("max_water_level")) {
      waterLevelSensor.setMax();
      sendSettingsJSON();
    }
    else if (request == F("zero_water_level")) {
      waterLevelSensor.setZero();
      sendSettingsJSON();
    }
#endif
    else if (request == F("set_program")) {
      handleCropProgramRequest();
      sendSystemData();
    }
    else if (request == F("drainage_drain_now")) {
      drainage.drainStart();
      sendSettingsJSON();
    }
    else if (request == F("drainage_set_automatic")) {
      drainage.drainStop();
      sendSettingsJSON();
    }
    else if (request == F("get_flowsensor_calibration")) {
      getFlowsensorCalibration();
    }
    else if (request == "start_flowsensor_calibration") {
      handleCalibrateFlowsensorsActionStart();
      getFlowsensorCalibration();
    }
    else if (request == F("stop_flowsensor_calibration")) {
      handleCalibrateFlowsensorsActionStop();
      getFlowsensorCalibration();
    }
    else if (request == F("get_message_log")) {
      sendMessageLog();
    }

    Serial.print(F("API call completed. Time taken: "));
    Serial.print(millis() - startMillis);
    Serial.println(F(" ms."));
  }
  APIRequest = false;
}

void sendJsonHeader() {
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), F("-1"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("application/json"), F(""));
}

void sendSystemData() {
  sendJsonHeader();
  server.sendContent_P(PSTR("{\"hydrodata\":\n"));
  sprintf_P(buff, PSTR("  {\"time\":\"%u\",\n"), now());
  server.sendContent(buff);
  sprintf_P(buff, PSTR("   \"waterTemp\":\"%.1f\",\n"), sensorData.waterTemp);
  server.sendContent(buff);
#ifdef USE_WATERLEVEL_SENSOR
  sprintf_P(buff, PSTR("   \"waterLevel\":\"%.1f\",\n"), sensorData.waterLevel);
  server.sendContent(buff);
#endif
  sprintf_P(buff, PSTR("   \"EC\":\"%.2f\",\n"), sensorData.EC);
  server.sendContent(buff);
  sprintf_P(buff, PSTR("   \"pH\":\"%.2f\",\n"), sensorData.pH);
  server.sendContent(buff);
  sprintf_P(buff, PSTR("   \"systemName\":\"%s\"\n"), sensorData.systemName);
  server.sendContent(buff);
  server.sendContent_P(PSTR("  },\n"
                            "  \"trayinfo\":{"));
  for (uint8_t i = 0; i < TRAYS; i++) {
    if (i > 0) {
      server.sendContent_P(PSTR(","));
    }
    server.sendContent_P(PSTR("\n"));
    sprintf_P(buff, PSTR("    \"%i\": {\n"), i + 1);
    server.sendContent(buff);
    server.sendContent_P(PSTR("     \"programState\":"));
    switch (trayInfo[i].programState) {
      case PROGRAM_UNSET:
        server.sendContent_P(PSTR("\"unset\""));
        break;

      case PROGRAM_SET:
        server.sendContent_P(PSTR("\"set\""));
        break;

      case PROGRAM_START:
        server.sendContent_P(PSTR("\"start\""));
        break;

      case PROGRAM_START_WATERING:
        server.sendContent_P(PSTR("\"start_watering\""));
        break;

      case PROGRAM_RUNNING:
        server.sendContent_P(PSTR("\"running\""));
        break;

      case PROGRAM_COMPLETE:
        server.sendContent_P(PSTR("\"complete\""));
        break;

      case PROGRAM_PAUSED:
      case PROGRAM_START_PAUSED:
        server.sendContent_P(PSTR("\"paused\""));
        break;

      case PROGRAM_ERROR:
        server.sendContent_P(PSTR("\"error\""));
        break;
    }
    server.sendContent_P(PSTR(",\n"));
    sprintf_P(buff, PSTR("     \"cropId\":\"%i\",\n"), trayInfo[i].cropId);
    server.sendContent(buff);
    sprintf_P(buff, PSTR("     \"startTime\":\"%u\",\n"), trayInfo[i].startTime);
    server.sendContent(buff);
#ifdef USE_GROWLIGHT
    sprintf_P(buff, PSTR("     \"darkDays\":\"%i\",\n"), trayInfo[i].darkDays);
    server.sendContent(buff);
#endif
    sprintf_P(buff, PSTR("     \"totalDays\":\"%i\",\n"), trayInfo[i].totalDays);
    server.sendContent(buff);
    sprintf_P(buff, PSTR("     \"wateringFrequency\":\"%i\",\n"), trayInfo[i].wateringFrequency);
    server.sendContent(buff);
    server.sendContent_P(PSTR("     \"present\":"));
    server.sendContent_P((bitRead(trayPresence, TRAY_PIN[i])) ? PSTR("1") : PSTR("0"));
    server.sendContent_P(PSTR("\n"
                              "    }"));
  }
  server.sendContent_P(PSTR("\n"
                            "  }\n"
                            "}"));
  server.sendContent("");
}

void sendMessageLog() {
  sendJsonHeader();
  logging.messagesJSON(&server);
}

void sendCropList() {
  sendJsonHeader();
  server.sendContent_P(PSTR("{\"cropdata\":[\n"));
  bool isFirst = true;
  for (uint8_t i = 0; i < N_CROPS; i++) {
    Crop crop = getCropData(i);
    if (crop.id > 0) {
      if (isFirst) {
        isFirst = false;
      }
      else {
        server.sendContent_P(PSTR(",\n"));
      }
      server.sendContent_P(PSTR("    {\"cropid\":\""));
      server.sendContent(itoa(crop.id, buff, 10));
      server.sendContent_P(PSTR("\",\n"
                                "     \"cropname\":\""));
      server.sendContent(crop.crop);
      server.sendContent_P(PSTR("\",\n"
                                "     \"cropdescription\":\""));

      server.sendContent(core.urlencode(crop.description));
      server.sendContent_P(PSTR("\"\n"
                                "    }"));
    }
  }
  server.sendContent_P(PSTR("\n"
                            "  ]\n"
                            "}"));
  server.sendContent("");
}

void sendSettingsJSON() {
  sendJsonHeader();
  server.sendContent_P(PSTR("{\n"));
  for (uint8_t i = 0; i < nSensors; i++) {
    if (sensors[i]->settingsJSON(&server)) {
      server.sendContent_P(PSTR(",\n"));
    }
  }
  if (fertiliser.settingsJSON(&server)) {
    server.sendContent_P(PSTR(",\n"));
  }
  if (pHMinus.settingsJSON(&server)) {
    server.sendContent_P(PSTR(",\n"));
  }
  if (drainage.settingsJSON(&server)) {
    server.sendContent_P(PSTR(",\n"));
  }
  if (logging.settingsJSON(&server)) {
    server.sendContent_P(PSTR(",\n"));
  }
  if (network.settingsJSON(&server)) {
    server.sendContent_P(PSTR(",\n"));
  }
  parameters.settingsJSON(&server);
  server.sendContent_P(PSTR("\n}"));
  server.sendContent("");
}
