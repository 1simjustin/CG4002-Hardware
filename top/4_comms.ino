/**
 * For now we work with Serial communication,
 * but we will eventually switch to Bluetooth
 * or WiFi for wireless communication.
 *
 * ID 0 is upper, ID 1 is lower
 */

void serialSensorsTask(void *parameter) {
    for (;;) {
        for (int i = 0; i < NUM_IMU; i++) {
            if (!imu_init[i]) {
                continue; // Skip if IMU failed to initialize
            }
            xSemaphoreTake(xIMUSemaphore[i], portMAX_DELAY);
        }
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
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
            Serial.println();
        }

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