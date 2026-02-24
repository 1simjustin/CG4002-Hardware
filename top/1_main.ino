void setup() {
  Serial.begin(115200);

  #if defined(TORSO_DEVICE) && defined(LEG_DEVICE)
  Serial.println("Multiple device types defined. Please define only one device type.");
  while (1) {
    delay(1000);
  }
  #endif

  // Initialize I2C and IMU
  i2c_setup();
  imu_setup();

  // Initialize Flex Sensor
  flex_setup();

  // Initialize Voltage Reader
  voltage_reader_setup();

  // Initialize Semaphores
  createSemaphores();

  // Create RTOS Tasks
  #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
  for (int i = 0; i < NUM_IMUS; i++) {
    const char* imuTaskName = ("IMUTask" + String(i)).c_str();
    xTaskCreatePinnedToCore(
      imuTask,                              // Task function
      imuTaskName,                          // Task name
      STACK_SIZE,                           // Stack size (bytes)
      (void *)i,                            // Parameters (pass IMU index)
      IMU_TASK_PRIORITY,                    // Priority
      &IMUTaskHandle[i],                    // Task handle
      SENSOR_CORE                           // Core 1 for sensors
    );

    const char* commsTaskName = ("CommsSensorsTask" + String(i)).c_str();
    xTaskCreatePinnedToCore(
      serialSensorsTask,                    // Task function
      commsTaskName,                        // Task name
      STACK_SIZE,                           // Stack size (bytes)
      (void *)i,                            // Parameters (pass sensor index)
      SENSORS_COMMS_TASK_PRIORITY,          // Priority
      &CommsSensorsTaskHandle[i],           // Task handle
      COMMS_CORE                            // Core 0 for comms
    );
  }

  for (int i = 0; i < NUM_FLEX; i++) {
    const char* flexTaskName = ("FlexTask" + String(i)).c_str();
    xTaskCreatePinnedToCore(
      flexTask,                             // Task function
      flexTaskName,                         // Task name
      STACK_SIZE,                           // Stack size (bytes)
      (void *)i,                            // Parameters (pass Flex index)
      FLEX_TASK_PRIORITY,                   // Priority
      &FlexTaskHandle[i],                   // Task handle
      SENSOR_CORE                           // Core 1 for sensors
    );
  }
  #endif

  xTaskCreatePinnedToCore(
    battTask,                // Task function
    "BatteryTask",           // Task name
    STACK_SIZE,              // Stack size (bytes)
    NULL,                    // Parameters
    BATT_TASK_PRIORITY,      // Priority
    &BatteryTaskHandle,      // Task handle
    SENSOR_CORE              // Core 1 for sensors
  );

  xTaskCreatePinnedToCore(
    serialBattTask,           // Task function
    "SerialBattTask",         // Task name
    STACK_SIZE,               // Stack size (bytes)
    NULL,                     // Parameters
    BATT_COMMS_TASK_PRIORITY, // Priority
    &CommsBattTaskHandle,     // Task handle
    COMMS_CORE                // Core 0 for comms
  );
}

void loop() {
  // Empty because FreeRTOS scheduler runs the task
}
