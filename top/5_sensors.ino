double radToDeg(double rad) {
  return rad * RAD_TO_DEG;
}

#ifdef TORSO_DEVICE
#endif

#ifdef LEG_DEVICE
TaskHandle_t IMUTaskHandle = NULL;
TaskHandle_t FlexTaskHandle = NULL;
TaskHandle_t BatteryTaskHandle = NULL;
#endif