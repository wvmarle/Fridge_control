////////////////////////////////////////////////////////////////////////
//
// Read the NTC probe.
//
////////////////////////////////////////////////////////////////////////

//const uint8_t NTCOversampling = 6; // 64x oversampling.
//const uint16_t NTCSeriesResistor = 10000;                   // The value of the NTC's series resistor in Ohm.
//const uint8_t NTC_VALUES = 71;
//
//struct calValue {
//  int8_t T;
//  uint16_t R;
//} const NTC[NTC_VALUES] = {
//  { -10, 55534},
//  { -9, 52595},
//  { -8, 49829},
//  { -7, 47225},
//  { -6, 44772},
//  { -5, 42462},
//  { -4, 40284},
//  { -3, 38230},
//  { -2, 36294},
//  { -1, 34466},
//  {0, 32741},
//  {1, 31113},
//  {2, 29575},
//  {3, 28122},
//  {4, 26749},
//  {5, 25451},
//  {6, 24223},
//  {7, 23061},
//  {8, 21962},
//  {9, 20921},
//  {10, 19936},
//  {11, 19002},
//  {12, 18118},
//  {13, 17280},
//  {14, 16485},
//  {15, 15731},
//  {16, 15016},
//  {17, 14337},
//  {18, 13693},
//  {19, 13081},
//  {20, 12500},
//  {21, 11948},
//  {22, 11423},
//  {23, 10925},
//  {24, 10451},
//  {25, 10000},
//  {26, 9570},
//  {27, 9162},
//  {28, 8773},
//  {29, 8403},
//  {30, 8051},
//  {31, 7715},
//  {32, 7395},
//  {33, 7090},
//  {34, 6799},
//  {35, 6522},
//  {36, 6257},
//  {37, 6005},
//  {38, 5764},
//  {39, 5534},
//  {40, 5315},
//  {41, 5105},
//  {42, 4905},
//  {43, 4713},
//  {44, 4530},
//  {45, 4355},
//  {46, 4188},
//  {47, 4028},
//  {48, 3875},
//  {49, 3729},
//  {50, 3589},
//  {51, 3455},
//  {52, 3326},
//  {53, 3203},
//  {54, 3086},
//  {55, 2973},
//  {56, 2865},
//  {57, 2761},
//  {58, 2662},
//  {59, 2566},
//  {60, 2475}
//};
//
//void initTemperatureSensor() {
//  pinMode(TEMP_PIN, INPUT);                             // Set pin to INPUT mode.
//
//}
//
//float readTemperatureSensor() {
//  uint32_t analogReading = 0;                           // Total of the ADC measurements done.
//  uint16_t temperature;                                 // The measured temperature *256.
//  analogRead(TEMP_PIN);                                 // Discard the first reading to allow ADC to settle.
//  for (uint16_t i = 0; i < (1 << NTCOversampling); i++) { // Take the required number of samples.
//    analogReading += analogRead(TEMP_PIN);
//  }
//  analogReading = (float)analogReading * vRef / 1014.0; // Compensate for the actual full scale of 1014 mV and 
//  //                                                    // the actual calibrated Vref.
//  uint16_t averageReading = analogReading >> NTCOversampling;
//  if (averageReading < 1000) {                          // NTC is present.
//    if (averageReading >= 203 || averageReading <= 867) { // NTC reading is in range.
//      uint16_t R = float(NTCSeriesResistor) * analogReading / ((uint32_t(1024) << NTCOversampling) - analogReading);
//      for (uint8_t i = 0; i < NTC_VALUES; i++) {        // Look up the resistance value in the calibration table.
//        if (R > NTC[i].R) {                             // Found our target. It's between this value and the previous one.
//          temperature = (uint32_t (NTC[i - 1].R - R) << 8) / (NTC[i - 1].R - NTC[i].R) + (NTC[i - 1].T << 8);
//          break;
//        }
//      }
//    }
//    else {
//      temperature = 100;
//    }
//  }
//  return temperature / 256.0;
//}
