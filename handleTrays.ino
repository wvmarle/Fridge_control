void handleTrays() {

  // Check some status bits to make sure we should be doing anything at all.
  if (bitRead(sensorData.systemStatus, STATUS_DRAINING_RESERVOIR) ||
      bitRead(sensorData.systemStatus, STATUS_FILLING_RESERVOIR) ||
      bitRead(sensorData.systemStatus, STATUS_DOOR_OPEN) ||
      bitRead(sensorData.systemStatus, STATUS_MAINTENANCE || // System is doing something that prevents us from watering,
      millis() < DRAIN_TIME )) {                            // or we've just started up - wait some time to ensure draining trays have really drained completely.
    if (bitRead(sensorData.systemStatus, STATUS_WATERING)) {// But we're currently watering! Make sure all pumps are off.
      for (uint8_t tray = 0; tray < TRAYS; tray++) {
        switch (wateringState[tray]) {
          case WATERING:
          case CALIBRATING:
            wateringState[tray] = DRAINING;                 // Tray was still filling up, draining now..
            mcp0.digitalWrite(PUMP_PIN[tray], LOW);         // Switch off the pump.
            wateringTime[tray] = millis();
            break;
        }
        switch (trayInfo[tray].programState) {
          case PROGRAM_START_WATERING:
            trayInfo[tray].programState = PROGRAM_START;
        }
      }
      bitClear(sensorData.systemStatus, STATUS_WATERING);   // Not watering any more.
    }
  }
  else {
    for (uint8_t i = 0; i < TRAYS; i++) {
      uint8_t tray = (lastWateredTray + i) % TRAYS;         // Continue with the next tray, not from the beginning.

      // Step 1: check whether this tray requires watering, and if so, set the status accordingly.
      switch (trayInfo[tray].programState) {
        case PROGRAM_START:                                 // A new program: start watering right away.
          if (bitRead(trayPresence, TRAY_PIN[tray])) {      // Only start the program if the tray is present.
            wateringState[tray] = WATERING_NEEDED;
            trayInfo[tray].programState = PROGRAM_START_WATERING; // Doing the initial watering run.
            wateringTime[tray] = millis();
            trayInfoChanged = true;
            sprintf_P(buff, PSTR("Starting program of tray %d."), tray + 1);
            logging.writeInfo(buff);
          }
          break;

        case PROGRAM_START_WATERING:                        // Wait for initial watering to be completed; then we're really running.
          if (wateringState[tray] == WATERING_IDLE) {
            trayInfo[tray].programState = PROGRAM_RUNNING;
            wateringState[tray] = WATERING_NEEDED;          // Repeat initial watering for proper wetting. Also the second time water level will be correct
            trayInfoChanged = true;                         // as now the residue was in the tray.
          }

        case PROGRAM_RUNNING:                               // Check on regular watering intervals.
        case PROGRAM_COMPLETE:
          if (wateringState[tray] == WATERING_IDLE &&
              trayInfo[tray].wateringFrequency > 0) {       // If interval set to 0: no watering for this tray, ever.
            uint32_t interval = (24 / trayInfo[tray].wateringFrequency) * 60 * 60 * 1000; // The watering interval for this tray in milliseconds.
            if (millis() - wateringTime[tray] > interval) { // Time to start watering this tray.
              wateringState[tray] = WATERING_NEEDED;
              wateringTime[tray] = millis();
            }
          }
          break;
      }

      // Step 2: Water the tray, if any needs watering.
      switch (wateringState[tray]) {
        case WATERING_IDLE:                                 // Check for flow: there should be none.
          if (millis() - wateringTime[tray] < DRAIN_TIME * 2) { // Wait a bit, make sure the tray is really empty.
            waterFlowSensorTicks[tray] = 0;
          }
          else if (millis() - lastFlowCheckTime[tray] > 60 * 1000) {
            if (waterFlowSensorTicks[tray] > 100) {
              sprintf_P(buff, PSTR("Watering 12: Unexpected flow detected at tray %d. Something appears amiss. Resetting the port extender, hoping to solve the issue."), tray + 1);
              logging.writeError(buff);
              MCP_init();
            }
            waterFlowSensorTicks[tray] = 0;
          }
          break;

        case WATERING_NEEDED:                               // This tray needs watering.
          if (bitRead(sensorData.systemStatus, STATUS_WATERING) == false &&            // No other trays are currently watering,
              bitRead(sensorData.systemStatus, STATUS_RESERVOIR_LEVEL_LOW) == false) { // and there's enough water in the reservoir.
            wateringState[tray] = WATERING;
            waterFlowSensorTicks[tray] = 0;                 // Reset the flow sensor of this tray.
            mcp0.digitalWrite(PUMP_PIN[tray], HIGH);        // Switch on the pump of this tray.
            pumpConfirmedRunning = false;                   // After 5 seconds check whether we see the flow meter active.
            wateringTime[tray] = millis();
            bitSet(sensorData.systemStatus, STATUS_WATERING);
          }
          break;

        case WATERING:                                      // Watering in progress: pump runs.
          if (waterFlowSensorTicks[tray] > flowSensorCount[tray]) { // If enough water pumped:
            mcp0.digitalWrite(PUMP_PIN[tray], LOW);         // Watering complete - switch off the pump of this tray.
            wateringTime[tray] = millis();                  // Check when we were finished.
            wateringState[tray] = DRAINING;                 // Allow the tray to drain.
            waterFlowSensorTicks[tray] = 0;                 // Look for backflow.
          }
          if (pumpConfirmedRunning == false &&              // After 10 seconds check whether the pump is running.
              millis() - wateringTime[tray] > 10 * 1000) {
            if (waterFlowSensorTicks[tray] < 10) {          // If running, we should see this in the flowSensorTicks value of this tray.
              sprintf_P(buff, PSTR("Watering 01: No flow detected when watering tray %d."), tray + 1);
              logging.writeWarning(buff);
              wateringState[tray] = DRAINING;
              wateringTime[tray] = millis();
              waterFlowSensorTicks[tray] = 0;               // Look for backflow.
              mcp0.digitalWrite(PUMP_PIN[tray], LOW);       // Switch off the pump.
              //              trayInfo[tray].programState = PROGRAM_ERROR; // Halt operation of this program: an error occurred.
              //              trayInfoChanged = true;
              uint32_t interval = (24 / trayInfo[tray].wateringFrequency) * 60 * 60 * 1000; // The watering interval for this tray in milliseconds.
              wateringTime[tray] = millis() - interval + 30 * 60 * 1000; // Try again in 30 minutes.
            }
            else {
              sprintf_P(buff, PSTR("Watering pump of tray %d is confirmed running."), tray + 1);
              logging.writeTrace(buff);
              pumpConfirmedRunning = true;
            }
          }
          if (millis() - wateringTime[tray] > 15 * 60 * 1000) { // After pumping for 15 minutes we should be long done!
            sprintf_P(buff, PSTR("Watering 10: After pumping for 15 minutes tray %d did not reach the required flow count. Stopping watering, halting program for this tray."), tray + 1);
            logging.writeError(buff);
            wateringState[tray] = DRAINING;
            wateringTime[tray] = millis();
            waterFlowSensorTicks[tray] = 0;                 // Look for backflow.
            mcp0.digitalWrite(PUMP_PIN[tray], LOW);       // Switch off the pump.
            trayInfo[tray].programState = PROGRAM_ERROR;  // Halt operation of this program: an error occurred.
            trayInfoChanged = true;
          }
          break;

        case DRAINING:                                      // Watering completed: tray drains for some time.
          if (millis() - wateringTime[tray] > 60 * 1000 &&  // After one minute: check for back flow, to detect whether there's water draining.
              waterFlowSensorTicks[tray] < 200 &&  // Expect at least 200 ticks on the flow meter.
              (trayInfo[tray].programState == PROGRAM_RUNNING ||
               trayInfo[tray].programState == PROGRAM_START_WATERING ||
               trayInfo[tray].programState == PROGRAM_COMPLETE)) {
            sprintf_P(buff, PSTR("Watering 11: Unusually low backflow detected from tray %d. Halting program of this tray, something appears amiss."), tray + 1);
            logging.writeError(buff);
            trayInfo[tray].programState = PROGRAM_ERROR;    // Halt operation of this program: an error occurred.
            trayInfoChanged = true;
          }
          if (millis() - wateringTime[tray] > DRAIN_TIME) {
            wateringState[tray] = WATERING_IDLE;            // Sequence complete.
            bitClear(sensorData.systemStatus, STATUS_WATERING);
          }
          break;
      }

      // Step 3: check the program status of the tray.
      // See if a running program has been completed.
      if (trayInfo[tray].programState == PROGRAM_RUNNING &&
          now() > trayInfo[tray].startTime) {               // Don't get set to "completed" immediately on startup, before time has been set.
        uint32_t elapsedTime = now() - trayInfo[tray].startTime;
        elapsedTime += 43200;                               // Make sure if the tray was started before noon, the first day is counted as a whole.
        if (int(elapsedTime / 86400) > trayInfo[tray].totalDays) {
          trayInfo[tray].programState = PROGRAM_COMPLETE;   // Program completed, crop is ready for harvest.
          trayInfoChanged = true;
        }
      }
      if (trayInfo[tray].programState == PROGRAM_PAUSED &&  // If the program has been paused
          wateringState[tray] == WATERING) {                // during watering:
        wateringState[tray] = DRAINING;                     // Set watering state to draining.
        wateringTime[tray] = millis();
        waterFlowSensorTicks[tray] = 0;                     // Look for backflow.
        mcp0.digitalWrite(PUMP_PIN[tray], LOW);             // Switch off the pump.
      }

      // Check the presence of the trays, and set the program state accordingly.
      if (bitRead(trayPresence, TRAY_PIN[tray]) == false) { // Tray not present, check for running program for this tray.
        switch (trayInfo[tray].programState) {
          case PROGRAM_START_WATERING:
            trayInfo[tray].programState = PROGRAM_START;
            trayInfoChanged = true;
            break;

          case PROGRAM_RUNNING:
            trayInfo[tray].programState = PROGRAM_PAUSED;
            trayInfoChanged = true;
            break;

          case PROGRAM_COMPLETE:
            trayInfo[tray].programState = PROGRAM_SET;
            trayInfoChanged = true;
            break;
        }
        switch (wateringState[tray]) {                      // If we're watering this tray: stop right now.
          case WATERING:
          case DRAINING:
          case CALIBRATING:                                 // Tray's pump is being calibrated. Abort this.
            mcp0.digitalWrite(PUMP_PIN[tray], LOW);
            wateringTime[tray] = millis();
            wateringState[tray] = WATERING_IDLE;
            break;
        }
      }
      yield();
    }

    // Calibration may be done for more than one tray at a time, or while programs are running.
    // Check whether any trays are watering/calibrating/draining, and set the lock bit accordingly.
    bool watering = false;
    for (uint8_t tray = 0; tray < TRAYS; tray++) {
      if (wateringState[tray] != WATERING_IDLE &&
          wateringState[tray] != WATERING_NEEDED) {
        watering = true;
        break;
      }
    }
    bitWrite(sensorData.systemStatus, STATUS_WATERING, watering);
  }
}
