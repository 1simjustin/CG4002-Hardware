void setup() {
  Serial.begin(115200);

  #if defined(TORSO_DEVICE) && defined(LEG_DEVICE)
  Serial.println("Multiple device types defined. Please define only one device type.");
  while (1) {
    delay(1000);
  }
  #endif

  // Initialize I2C
  i2c_setup();

  // Initialize Voltage Reader
  voltage_reader_setup();

  // Initialize Semaphores
  createSemaphores();

  delay(10);

  // Create RTOS Tasks
  #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
  for (int i = 0; i < NUM_IMU; i++) {
    char imuTaskName[20];
    sprintf(imuTaskName, "IMUTask%d", i);
    result = xTaskCreatePinnedToCore(
      imuTask,                              // Task function
      imuTaskName,                          // Task name
      STACK_SIZE,                           // Stack size (bytes)
      (void *)i,                            // Parameters (pass IMU index)
      IMU_TASK_PRIORITY,                    // Priority
      &IMUTaskHandle[i],                    // Task handle
      SENSOR_CORE                           // Core 1 for sensors
    );
    if (result != pdPASS) {
      Serial.print("Failed to create task: ");
      Serial.println(imuTaskName);
    }

    #if defined(ENABLE_SENSOR_COMMS)
    char commsTaskName[25];
    sprintf(commsTaskName, "SerialSensorsTask%d", i);
    result = xTaskCreatePinnedToCore(
      serialSensorsTask,                    // Task function
      commsTaskName,                        // Task name
      STACK_SIZE,                           // Stack size (bytes)
      (void *)i,                            // Parameters (pass sensor index)
      SENSORS_COMMS_TASK_PRIORITY,          // Priority
      &CommsSensorsTaskHandle[i],           // Task handle
      COMMS_CORE                            // Core 0 for comms
    );
    if (result != pdPASS) {
      Serial.print("Failed to create task: ");
      Serial.println(commsTaskName);
    }
    #endif
  }

  for (int i = 0; i < NUM_FLEX; i++) {
    char flexTaskName[20];
    sprintf(flexTaskName, "FlexTask%d", i);
    result = xTaskCreatePinnedToCore(
      flexTask,                             // Task function
      flexTaskName,                         // Task name
      STACK_SIZE,                           // Stack size (bytes)
      (void *)i,                            // Parameters (pass Flex index)
      FLEX_TASK_PRIORITY,                   // Priority
      &FlexTaskHandle[i],                   // Task handle
      SENSOR_CORE                           // Core 1 for sensors
    );
    if (result != pdPASS) {
      Serial.print("Failed to create task: ");
      Serial.println(flexTaskName);
    }
  }
  #endif

  result = xTaskCreatePinnedToCore(
    battTask,                // Task function
    "BatteryTask",           // Task name
    STACK_SIZE,              // Stack size (bytes)
    NULL,                    // Parameters
    BATT_TASK_PRIORITY,      // Priority
    &BatteryTaskHandle,      // Task handle
    SENSOR_CORE              // Core 1 for sensors
  );
  if (result != pdPASS) {
    Serial.println("Failed to create task: BatteryTask");
  }

  result = xTaskCreatePinnedToCore(
    serialBattTask,           // Task function
    "SerialBattTask",         // Task name
    STACK_SIZE,               // Stack size (bytes)
    NULL,                     // Parameters
    BATT_COMMS_TASK_PRIORITY, // Priority
    &CommsBattTaskHandle,     // Task handle
    COMMS_CORE                // Core 0 for comms
  );
  if (result != pdPASS) {
    Serial.println("Failed to create task: SerialBattTask");
  }

  #if defined(DEBUG)
  result = xTaskCreatePinnedToCore(
    monitorTask,              // Task function
    "MonitorTask",            // Task name
    STACK_SIZE,               // Stack size (bytes)
    NULL,                     // Parameters
    1,                        // Priority (lowest)
    &MonitorTaskHandle,       // Task handle
    COMMS_CORE                // Core 0 for comms
  );
  if (result != pdPASS) {
    Serial.println("Failed to create task: MonitorTask");
  }
  #endif

  result = xTaskCreatePinnedToCore(
    battDispTask,             // Task function
    "BatteryDisplayTask",     // Task name
    STACK_SIZE,               // Stack size (bytes)
    NULL,                     // Parameters
    ACTUATOR_TASK_PRIORITY,   // Priority
    NULL,                     // Task handle (not needed)
    SENSOR_CORE               // Core 1 for actuators
  );
}

void loop() {
  // Empty because FreeRTOS scheduler runs the task
}
