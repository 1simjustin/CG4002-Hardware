double radToDeg(double rad) {
  return rad * RAD_TO_DEG;
}

void imuTask(void *parameter) {
  #ifdef TORSO_DEVICE
  #endif

  #ifdef LEG_DEVICE
  for (;;) {
    mpu.getEvent(&a, &g, NULL);
    #ifdef USE_FUSION
    double gx = radToDeg(g.gyro.x);
    double gy = radToDeg(g.gyro.y);
    double gz = radToDeg(g.gyro.z);
    filter.updateIMU(gx, gy, gz, a.acceleration.x, a.acceleration.y, a.acceleration.z);
    #endif

    vTaskDelay(IMU_PERIOD_MS / portTICK_PERIOD_MS);
  }
  #endif
}

void flexTask(void *parameter) {
  #ifdef TORSO_DEVICE
  #endif

  #ifdef LEG_DEVICE
  for (;;) {
    for (int i = 0; i < NUM_FLEX; i++) {
      flex_readings[i] = analogRead(FLEX_PINS[i]);
    }

    vTaskDelay(FLEX_PERIOD_MS / portTICK_PERIOD_MS);
  }
  #endif
}

void battTask(void *parameter) {
  for (;;) {
    int adc_value = analogRead(BATTERY_PIN);
    batt_voltage = (adc_value / ADC_MAX) * ADC_REF_VOLTAGE * VOLTAGE_DIVIDER_RATIO;

    // Clamp voltage to reference voltage to avoid erroneous readings
    // Especially important since voltage from USB reads as 5V which is above the ADC reference voltage
    batt_voltage = min(batt_voltage, ADC_REF_VOLTAGE);

    vTaskDelay(BATT_PERIOD_MS / portTICK_PERIOD_MS);
  }
}