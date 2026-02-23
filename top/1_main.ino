void setup() {
  Serial.begin(115200);

  // Initialize I2C and IMU
  i2c_setup();
  imu_setup();

  // Initialize Flex Sensor
  flex_setup();

  // Initialize Voltage Reader
  voltage_reader_setup();

  // Create RTOS Tasks
  #ifdef TORSO_DEVICE
  #endif

  #ifdef LEG_DEVICE
  xTaskCreatePinnedToCore(
    imuTask,              // Task function
    "IMUTask",            // Task name
    STACK_SIZE,           // Stack size (bytes)
    NULL,                 // Parameters
    IMU_TASK_PRIORITY,    // Priority
    &IMUTaskHandle,       // Task handle
    SENSOR_CORE           // Core 1 for sensors
  );

  xTaskCreatePinnedToCore(
    flexTask,             // Task function
    "FlexTask",           // Task name
    STACK_SIZE,           // Stack size (bytes)
    NULL,                 // Parameters
    FLEX_TASK_PRIORITY,   // Priority
    &FlexTaskHandle,      // Task handle
    SENSOR_CORE           // Core 1 for sensors
  );
  #endif

  xTaskCreatePinnedToCore(
    battTask,             // Task function
    "BatteryTask",        // Task name
    STACK_SIZE,           // Stack size (bytes)
    NULL,                 // Parameters
    BATT_TASK_PRIORITY,   // Priority
    &BatteryTaskHandle,   // Task handle
    SENSOR_CORE           // Core 1 for sensors
  );
}

void loop() {
  // Empty because FreeRTOS scheduler runs the task
}
