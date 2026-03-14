/**
 * For now we work with Serial communication,
 * but we will eventually switch to Bluetooth
 * or WiFi for wireless communication.
 *
 * ID 0 is upper, ID 1 is lower
 */

void serialSensorsTask(void *parameter) {
    bool any_imu_init = false;
    while (!any_imu_init) {
        for (int i = 0; i < NUM_IMU; i++) {
            if (imu_init[i]) {
                any_imu_init = true;
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
            xSemaphoreTake(xIMUSemaphore[i], portMAX_DELAY);
        }
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            if (!imu_init[sensor_id]) {
                continue; // Skip if IMU failed to initialize
            }

            Serial.print("IMU ID: ");
            Serial.print(sensor_id);

            Serial.print(" | Acceleration (m/s^2): X=");
            Serial.print(data_out[sensor_id].avg_x);
            Serial.print(" Y=");
            Serial.print(data_out[sensor_id].avg_y);
            Serial.print(" Z=");
            Serial.print(data_out[sensor_id].avg_z);

            Serial.print(" | IMU Gyro (deg/s): R=");
            Serial.print(data_out[sensor_id].avg_roll);
            Serial.print(" P=");
            Serial.print(data_out[sensor_id].avg_pitch);
            Serial.print(" Y=");
            Serial.print(data_out[sensor_id].avg_yaw);
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

void commsSensorsTask(void *parameter) {}

void commsBattTask(void *parameter) {}