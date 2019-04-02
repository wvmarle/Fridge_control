
/***********************************************************************************************************
  /calibrate_ec: calibrate the EC sensor.
*/
void handleCalibrateEC() {
  network.htmlResponse(&server);
  network.htmlPageHeader(&server, false);
  ECSensor.getCalibrationHtml(&server);
  network.htmlPageFooter(&server);
}

// /calibrate_ec: calibrate the EC sensor.
void handleCalibrateECAction() {
  ECSensor.doCalibrationAction(&server);
  handleCalibrateEC();
}
