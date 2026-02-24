// FLEX ID 0: Left Arm
// FLEX ID 1: Right Arm

void flex_setup() {
  #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
  for (int i = 0; i < NUM_FLEX; i++) {
    pinMode(FLEX_PINS[i], INPUT);
  }
  #endif
}

double processFlexReading(int raw_reading) {
  // Placeholder for actual processing logic
  // For now we just return the raw reading, but we can apply calibration or mapping to angles later
  return (double)raw_reading;
}

void flexTask(void *parameter) {
  int imu_id = (int)parameter;

  for (;;) {
    #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
    flex_readings[imu_id] = analogRead(FLEX_PINS[imu_id]);
    flex_output[imu_id] = processFlexReading(flex_readings[imu_id]);

    xSemaphoreGive(xFlexSemaphore[imu_id]);
    vTaskDelay(FLEX_PERIOD_MS / portTICK_PERIOD_MS);
    #endif
  }
}