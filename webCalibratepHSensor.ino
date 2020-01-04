
/***********************************************************************************************************
  /calibrate_ph: calibrate the pH sensor.
*/
void handleCalibratepH() {
  network.htmlResponse();
  network.htmlPageHeader(false);
  pHSensor.getCalibrationHtml(&server);
  network.htmlPageFooter();
}

void handleCalibratepHAction() {
  pHSensor.doCalibrationAction(&server);
  network.redirectTo("/calibrate_ph");
}
