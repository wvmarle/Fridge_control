
/***********************************************************************************************************
  /calibrate_ec: calibrate the EC sensor.
*/
void handleCalibrateEC() {
  network.htmlResponse();
  network.htmlPageHeader(false);
  ECSensor.getCalibrationHtml(&server);
  network.htmlPageFooter();
}

// /calibrate_ec: calibrate the EC sensor.
void handleCalibrateECAction() {
  ECSensor.doCalibrationAction(&server);
  network.redirectTo("/calibrate_ec");
}
