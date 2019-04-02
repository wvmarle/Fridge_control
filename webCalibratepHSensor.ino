
/***********************************************************************************************************
  /calibrate_ph: calibrate the pH sensor.
*/
void handleCalibratepH() {
  network.htmlResponse(&server);
  network.htmlPageHeader(&server, false);
  pHSensor.getCalibrationHtml(&server);
  network.htmlPageFooter(&server);
}

void handleCalibratepHAction() {
  pHSensor.doCalibrationAction(&server);
  handleCalibratepH();
}
