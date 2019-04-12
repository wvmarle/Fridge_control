//********************************************************************************************************************
// Read the data of the crop given by the id.
Crop getCropData(uint8_t id) {
  Crop crop;
  sprintf_P(buff, PSTR("/%d.json"), id);                    // The file name should be the id number with .json extension.
  File f = SPIFFS.open(buff, "r");
  if (!f) {                                                 // No file found with this name. Returning.
    crop.id = 0;
    return crop;
  }
  uint16_t s = f.size();
  char json[s + 1];
  f.read((uint8_t*)json, s);
  json[s] = 0;                                              // null terminator.
  StaticJsonBuffer<600> jsonBuffer;                         // Create a buffer big enough to contain the complete file.
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {                                    // Invalid json format.
    crop.id = 0;
    return crop;
  }
  crop.id = id;
  strlcpy(crop.crop, root["crop"], 32);
  strlcpy(crop.description, root["description"], 512);
  crop.darkDays = root["dark_days"];
  crop.totalDays = root["total_days"];
  crop.wateringFrequency = root["watering"];
  return crop;
}

//********************************************************************************************************************
// Read the program data from a set of key/value pairs.
void getProgramData(String keys[], uint32_t values[], uint8_t nArgs, uint8_t *id, uint8_t tray, bool *argsValid) {
  bool valid = false;
  for (uint8_t i = 0; i < nArgs; i++) {                     // Search for the crop number.
    if (keys[i] == "crop_id") {                             // The key that contains the crop id (the plant this is about).
      if (haveCropId(values[i])) {                          // Make sure we have a crop with this id.
        *id = values[i];                                    // Record the crop id.
        valid = true;
      }
      break;                                                // We're done here.
    }
  }
  if (valid) {
    if (*id == 255) {                                       // Custom crop set, expect to get program data.
      uint8_t totalDays = 0;
      uint8_t darkDays = 0;
      uint16_t wateringFrequency = 0;
      for (uint8_t i = 0; i < nArgs; i++) {                 // Search for the other arguments.
        if (keys[i] == "total_days") {                      // Number of days the program should last.
          if (values[i] > 60) {                             // Sensible limit for the total program - normal is 5-20 days.
            valid = false;
          }
          else {
            totalDays = values[i];
          }
        }
        if (keys[i] == "dark_days") {                       // Number of days the lights are kept off.
          if (values[i] > 30) {                             // Sensible limit for the days without light - normal is 1-5 days.
            valid = false;
          }
          else {
            darkDays = values[i];
          }
        }
        if (keys[i] == "watering_frequency") {              // Number of times a day the watering is done.
          valid = isValidFrequency(values[i]);
          if (valid) {
            wateringFrequency = values[i];
          }
        }
      }
      if (totalDays < darkDays) {                           // Can't be kept dark longer than the total program duration.
        darkDays = totalDays;
      }
      if (valid) {                                          // Valid data - custom crop. Store it in the tray info.
        trayInfo[tray].totalDays = totalDays;
#ifdef USE_GROWLIGHT
        trayInfo[tray].darkDays = darkDays;
#endif
        trayInfo[tray].wateringFrequency = wateringFrequency;
        trayInfoChanged = true;
      }
    }
    else {
      Crop c = getCropData(*id);                            // It's a preset crop; read the stored data.
#ifdef USE_GROWLIGHT
      trayInfo[tray].darkDays = c.darkDays;
#endif
      trayInfo[tray].totalDays = c.totalDays;
      trayInfo[tray].wateringFrequency = c.wateringFrequency;
      trayInfoChanged = true;
    }
  }
  *argsValid = valid;
}

//********************************************************************************************************************
// Check whether we have a specific crop id.
bool haveCropId(uint8_t id) {
  if (id == 0 || id == 255) return true;                    // 255 = custom crop. We always have that.
  for (uint8_t i = 0; i < 254; i++) {
    if (id = cropId[i].cropId) return true;                 // We have this crop id in the list.
  }
  return false;                                             // Not found.
}

//********************************************************************************************************************
// Read the list of available crop ids.
void getCropIds() {
  uint8_t i = 0;

  // Count the files first, so we can create an appropriate sized array.
  Dir dir = SPIFFS.openDir("/");                            // Open the root of the SPIFFS.
  while (dir.next()) {                                      // Iterate over all the files that are present.
    String fileName = dir.fileName();                       // Get the file name.
    if (fileName.substring(fileName.length() - 5) == ".json") {
      uint32_t id = fileName.substring(1, fileName.length() - 5).toInt(); // The file name is the crop id number: 1-254 (allow for other numbers by using a larger type).
      if (id > 0 && id < 255) {                             // Valid ids are 1-254, as 0 and 255 are reserved.
        cropId[i].cropId = (uint8_t)id;                     // Store the value in the array.
        Crop crop = getCropData(id);
        strlcpy(cropId[i].crop, crop.crop, 32);
        i++;
      }
    }
  }
}

//********************************************************************************************************************
// Set the crop drop-down menu for selecting crops, or the program details if a program is selected already.
// Column 2 of the tray info table.
void cropProgramHtml(uint8_t tray) {

  // First line: the name of the crop set - if any, and a drop-down menu to set the crop if needed.
  if (trayInfo[tray].programState == PROGRAM_UNSET) {       // No program set.
    server.sendContent_P(PSTR("No program set. "));
  }
  else if (trayInfo[tray].cropId == 255) {                  // Custom program set.
    server.sendContent_P(PSTR("Custom crop programme "));
  }
  else {                                                    // A crop has been set - show the name.
    for (uint8_t i = 0; i < N_CROPS; i++) {
      if (cropId[i].cropId == trayInfo[tray].cropId) {      // Find the set crop from the list.
        strcpy(buff, cropId[i].crop);
        strcat(buff, " ");                                  // Make sure we never send an empty string, which the web client interprets as EOF.
        server.sendContent(buff);
        break;
      }
    }
  }

  // Add the drop-down menu to set the crop, if appropriate for the program status.
  if (trayInfo[tray].programState == PROGRAM_UNSET ||
      trayInfo[tray].programState == PROGRAM_COMPLETE) {
    server.sendContent_P(PSTR("\n\
    <select name=\"crop_id\">\n"));
    if (trayInfo[tray].programState == PROGRAM_UNSET) {
      server.sendContent_P(PSTR("\
      <option value=\"\" selected disabled>Select crop</option>\n"));
    }
    server.sendContent_P(PSTR("\
      <option value=\"255\""));
    if (trayInfo[tray].cropId == 255) {
      server.sendContent_P(PSTR(" selected"));
    }
    server.sendContent_P(PSTR(">Custom crop programme</option>\n"));
    for (uint8_t i = 0; i < N_CROPS; i++) {
      if (cropId[i].cropId > 0) {
        server.sendContent_P(PSTR("\
      <option value=\""));
        server.sendContent(itoa(cropId[i].cropId, buff, 10));
        if (trayInfo[tray].cropId == cropId[i].cropId) {
          server.sendContent_P(PSTR("\" selected>"));
        }
        else {
          server.sendContent_P(PSTR("\">"));
        }
        server.sendContent(cropId[i].crop);
        server.sendContent_P(PSTR("</option>\n"));
      }
    }
    server.sendContent_P(PSTR("\
            </select>"));
  }
  server.sendContent_P(PSTR("<br>\n"));

  // Add the details of the selected programme, if any.
  if (trayInfo[tray].programState != PROGRAM_UNSET) {
    if (trayInfo[tray].cropId == 255 &&                     // Show custom crop program settings as input fields for certain program status.
        (trayInfo[tray].programState == PROGRAM_SET ||
         trayInfo[tray].programState == PROGRAM_COMPLETE)) {
      server.sendContent_P(PSTR("\
            Total program: <input type = \"text\" name=\"total_days\" size=\"2\" value=\""));
      server.sendContent(itoa(trayInfo[tray].totalDays, buff, 10));
      server.sendContent_P(PSTR("\"> days.<br>\n"));
#ifdef USE_GROWLIGHT
      server.sendContent_P(PSTR("\
            Growlight after: <input type=\"text\" name=\"dark_days\" size=\"2\" value=\""));
      server.sendContent(itoa(trayInfo[tray].darkDays, buff, 10));
      server.sendContent_P(PSTR("\"> days.<br>\n"))
#endif
      server.sendContent_P(PSTR("\
            Watering: <select name=\"watering_frequency\">\n"));
      for (uint8_t i = 0; i < N_FREQUENCIES; i++) {
        server.sendContent_P(PSTR("\
              <option value=\""));
        server.sendContent(itoa(WATERING_FREQUENCIES[i], buff, 10));
        server.sendContent_P(PSTR("\""));
        if (WATERING_FREQUENCIES[i] == trayInfo[tray].wateringFrequency) {
          server.sendContent_P(PSTR(" selected"));
        }
        server.sendContent_P(PSTR(">"));
        if (WATERING_FREQUENCIES[i] == 0) {
          server.sendContent_P(PSTR("never"));
        }
        else {
          server.sendContent(itoa(WATERING_FREQUENCIES[i], buff, 10));
          if (WATERING_FREQUENCIES[i] == 1) {
            server.sendContent_P(PSTR(" time daily"));
          }
          else {
            server.sendContent_P(PSTR(" times daily"));
          }
        }
        server.sendContent_P(PSTR("</option>\n"));
      }
      server.sendContent_P(PSTR("\
            </select>\n\
            <input type=\"hidden\" name=\"crop_id\" value=\""));
      server.sendContent(itoa(trayInfo[tray].cropId, buff, 10));
      server.sendContent_P(PSTR("\">"));
    }
    else {
      server.sendContent_P(PSTR("\
          Total program: "));
      server.sendContent(itoa(trayInfo[tray].totalDays, buff, 10));
#ifdef USE_GROWLIGHT
      server.sendContent_P(PSTR(" days.<br>\n\
          Growlight after: "));
      server.sendContent(itoa(trayInfo[tray].darkDays, buff, 10));
      server.sendContent_P(PSTR(" days.<br>\n\
          Watering: "));
#else
      server.sendContent_P(PSTR(" days.<br>\n\
          Watering: "));
#endif
      server.sendContent(itoa(trayInfo[tray].wateringFrequency, buff, 10));
      server.sendContent_P(PSTR(" times daily."));
    }
  }
}

//********************************************************************************************************************
// Take the datetime as seconds from epoch, convert to local time, and return a nicely formatted date & time string.
void datetime(char* buff, time_t t) {

  if (t == 0 || t == -1) {
    strcpy_P(buff, PSTR("n/a"));
  }
  else {
    t += sensorData.timezone * 60 * 60;
    sprintf_P(buff, PSTR("%04d/%02d/%02d %02d:%02d:%02d"), year(t), month(t), day(t), hour(t), minute(t), second(t));
  }
}

//********************************************************************************************************************
// Get the current date and time, if available.
void datetime(char* buff) {
  if (timeStatus() != timeNotSet) {
    datetime(buff, now());
  }
  else {
    strcpy_P(buff, PSTR("n/a"));
  }
}

//********************************************************************************************************************
// Convert a number of seconds to days/hours, hours/minutes or just minutes.
void daysHours(char* buff, time_t t) {
  if (t == 0 || t == -1) {
    strcpy_P(buff, PSTR("n/a"));
  }
  if (day(t) - 1) {
    sprintf_P(buff, PSTR("%02d days, %02d hours."), day(t) - 1, hour(t));
  }
  else if (hour(t)) {
    sprintf_P(buff, PSTR("%02d hours, %02d mins."), hour(t), minute(t));
  }
  else {
    sprintf_P(buff, PSTR("%02d mins."), minute(t));
  }

}

//********************************************************************************************************************
// Check whether the watering frequency is valud.
bool isValidFrequency(uint8_t f) {
  for (uint8_t j = 0; j < N_FREQUENCIES; j++) {             // Iterate over the possible frequencies, and see whether we have a valid value.
    if (f == WATERING_FREQUENCIES[j]) {
      return true;
    }
  }
  return false;
}
