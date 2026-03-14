void serialSensorsTask(void *parameter) {
    imu_reading_t sensor_readings[NUM_IMU] = {0};
    bool any_imu_ready = false;

    while (!any_imu_ready) {
        for (int i = 0; i < NUM_IMU; i++) {
            if (imu_init[i]) {
                any_imu_ready = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
#if defined(DEBUG)
    Serial.println("Serial Sensors Task Started");
#endif

    for (;;) {
        for (int i = 0; i < NUM_IMU; i++) {
            if (!imu_init[i]) {
                continue; // Skip if IMU failed to initialize
            }
            xQueueReceive(xIMUQueue[i], &sensor_readings[i], portMAX_DELAY);
        }
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            if (!imu_init[sensor_id]) {
                continue; // Skip if IMU failed to initialize
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