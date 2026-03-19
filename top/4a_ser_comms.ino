void serialSensorsTask(void *parameter) {
    imu_reading_t sensor_readings[NUM_IMU] = {0};

    // Wait until at least any IMU is initialized before starting to send data
    xEventGroupWaitBits(xIMUEventGroup, // Event Group Handle
                        IMU_FLAG_BITS,  // Bits to wait for (IMUs initialized)
                        pdFALSE,        // Do not clear bits on exit
                        pdFALSE,        // Any bit
                        portMAX_DELAY   // Wait indefinitely
    );

#if defined(DEBUG)
    Serial.println("Serial Sensors Task Started");
#endif

    for (;;) {
        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            // Skip if IMU failed to initialize
            if (xEventGroupGetBits(xIMUEventGroup) & (1 << sensor_id)) {
                xQueueReceive(
                    xIMUQueue[sensor_id],        // Queue handle
                    &sensor_readings[sensor_id], // Buffer to receive data
                    portMAX_DELAY                // Wait indefinitely
                );
            } else {
                // If IMU failed to initialize, set readings to 0
                sensor_readings[sensor_id] = {0};
            }
        }
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            // Skip if IMU failed to initialize
            if ((xEventGroupGetBits(xIMUEventGroup) & (1 << sensor_id)) == 0) {
                continue;
            }

            Serial.print("IMU ID: ");
            Serial.print(sensor_id);

            Serial.print(" | Acceleration (m/s^2): X=");
            Serial.print(sensor_readings[sensor_id].x);
            Serial.print(" Y=");
            Serial.print(sensor_readings[sensor_id].y);
            Serial.print(" Z=");
            Serial.print(sensor_readings[sensor_id].z);

            Serial.print(" | IMU Gyro (deg/s): R=");
            Serial.print(sensor_readings[sensor_id].roll);
            Serial.print(" P=");
            Serial.print(sensor_readings[sensor_id].pitch);
            Serial.print(" Y=");
            Serial.print(sensor_readings[sensor_id].yaw);
            Serial.println();
        }
        Serial.println("--------------------------------------------------");

        xSemaphoreGive(xSerialMutex);
    }
}

void serialBattTask(void *parameter) {
    for (;;) {
        xSemaphoreTake(xBattSemaphore, portMAX_DELAY);
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);

        Serial.print("Battery Voltage: ");
        Serial.print(batt_voltage);
        Serial.print(" V | ");
        Serial.print(batt_percentage);
        Serial.println("%");
        xSemaphoreGive(xSerialMutex);
    }
}