/**
 * For now we work with Serial communication,
 * but we will eventually switch to Bluetooth
 * or WiFi for wireless communication.
 */

void serialSensorsTask(void *parameter) {
  // Sensor ID corresponds to IMU ID and Flex ID (if applicable)
  // Chest IMU does not have a corresponding Flex sensor,
  // so we can use the sensor ID to determine whether to print Flex data
  int sensor_id = (int)parameter;

  #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
  for (;;) {
    xSemaphoreTake(xIMUSemaphore[sensor_id], portMAX_DELAY);
    if (sensor_id < NUM_FLEX) {
      xSemaphoreTake(xFlexSemaphore[sensor_id], portMAX_DELAY);
    }

    Serial.print("IMU ID: ");
    Serial.print(sensor_id);

    Serial.print(" | Acceleration (m/s^2): X=");
    Serial.print(x_out[sensor_id]);
    Serial.print(" Y=");
    Serial.print(y_out[sensor_id]);
    Serial.print(" Z=");
    Serial.print(z_out[sensor_id]);

    Serial.print(" | IMU Gyro (deg/s): R=");
    Serial.print(roll_out[sensor_id]);
    Serial.print(" P=");
    Serial.print(pitch_out[sensor_id]);
    Serial.print(" Y=");
    Serial.print(yaw_out[sensor_id]);

    if (sensor_id < NUM_FLEX) {
      Serial.print(" | Flex Reading: ");
      Serial.print(flex_output[sensor_id]);
    }
    Serial.println();
  }
  #endif
}

void serialBattTask(void *parameter) {
  for (;;) {
    xSemaphoreTake(xBattSemaphore, portMAX_DELAY);

    Serial.print("Battery Voltage: ");
    Serial.print(batt_voltage);
    Serial.print(" V | ");
    Serial.print(batt_percentage);
    Serial.println("%");
  }
}

void commsSensorsTask(void *parameter) {
}

void commsBattTask(void *parameter) {
}