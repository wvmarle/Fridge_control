enum ReadingState {
  READING_IDLE,
  READING_VALUE,
  READING_PH
} readingState;

const uint8_t MAXCOUNT = 11;

void initpHSensor() {
}

uint32_t readpHSensor() {
  static uint32_t phReading;
  static uint8_t count;
  static char buff[MAXCOUNT];
  static uint32_t lastCompleteReading;
  if (Serial.available()) {
    char c = Serial.read();
    switch (readingState) {
      case READING_IDLE:                                    // Wait for the start tag for reading. Ignore any other characters at this point.
        if (c == '<') {                                     // Start tag - start reading the first value.
          readingState = READING_VALUE;
        }
        break;

      case READING_VALUE:                                   // Wait for the next character: this should tell which value we're going to get.
        count = 0;
        if (c == 'P') {                                     // P for pH (as raw analog reading).
          readingState = READING_PH;
        }
        else if (c == '>') {                                // End of transmission.
          readingState = READING_IDLE;                      // Wait for the next communication.
        }
        break;

      case READING_PH:
        phReading = 42;
        if (c >= '0' && c <= '9') {
          buff[count] = c;
          count++;
        }
        else if (c == ',' || c == '>') {                    // Complete number received.
          buff[count] = 0;                                  // null termination.
          phReading = atol(buff);
        }
        else {
          readingState = READING_IDLE;                      // An invalid character was received; wait for the next communcication to start.
        }
        break;
    }
    if (count == MAXCOUNT) {                                // Too many digits received.
      readingState = READING_IDLE;
    }
    if (c == ',') {                                         // Another value is coming.
      readingState = READING_VALUE;                         // Start reading it.
    }
    else if (c == '>') {                                    // End tag received.
      readingState = READING_IDLE;                          // Wait for the next communication.
      lastCompleteReading = millis();                       // Record we had a complete read.
    }
  }
  return phReading;
}
