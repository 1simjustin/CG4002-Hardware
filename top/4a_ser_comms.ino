void serialSensorsTask(void *parameter) {
    imu_reading_t sensor_readings[NUM_IMU] = {0};
    EventBits_t expected_imu_bits;
    uint32_t received_imu_mask;

    // Wait until at least any IMU is initialized before starting to send data
    xEventGroupWaitBits(xSystemEventGroup, // Event Group Handle
                        IMU_FLAG_BITS, // Bits to wait for (IMUs initialized)
                        pdFALSE,       // Do not clear bits on exit
                        pdFALSE,       // Any bit
                        portMAX_DELAY  // Wait indefinitely
    );

#if defined(DEBUG)
    Serial.println("Serial Sensors Task Started");
#endif

    for (;;) {
        // Check if system is running
        if ((xEventGroupGetBits(xSystemEventGroup) & COMMS_RUNNING_FLAG_BIT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_DELAY_MS));
            continue;
        }

        // Collect data from all initialized IMUs (non-blocking)
        expected_imu_bits = xEventGroupGetBits(xSystemEventGroup) & IMU_FLAG_BITS;
        received_imu_mask = 0;

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            if (expected_imu_bits & (1 << sensor_id)) {
                if (xQueueReceive(xIMUQueue[sensor_id],
                                  &sensor_readings[sensor_id], 0) == pdTRUE) {
                    received_imu_mask |= (1 << sensor_id);
                }
            } else {
                memset(&sensor_readings[sensor_id], 0,
                       sizeof(sensor_readings[sensor_id]));
            }
        }

        // Print data if all initialized IMUs have new data
        if (expected_imu_bits > 0 && received_imu_mask == expected_imu_bits) {
            xSemaphoreTake(xSerialMutex, portMAX_DELAY);
            for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
                if (!(expected_imu_bits & (1 << sensor_id)))
                    continue;

                Serial.print("ID: ");
                Serial.print(sensor_id);

                Serial.print(" | ");
                Serial.print(sensor_readings[sensor_id].x);
                Serial.print(" ");
                Serial.print(sensor_readings[sensor_id].y);
                Serial.print(" ");
                Serial.print(sensor_readings[sensor_id].z);

                Serial.print(" | ");
                Serial.print(sensor_readings[sensor_id].roll);
                Serial.print(" ");
                Serial.print(sensor_readings[sensor_id].pitch);
                Serial.print(" ");
                Serial.print(sensor_readings[sensor_id].yaw);
                Serial.println();
            }
            xSemaphoreGive(xSerialMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_DELAY_MS));
    }
}

void serialBattTask(void *parameter) {
    batt_reading_t reading;
    for (;;) {
        xQueueReceive(xBattSerialQueue, &reading, portMAX_DELAY);
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);

        Serial.print("Battery Voltage: ");
        Serial.print(reading.voltage);
        Serial.print(" V | ");
        Serial.print(reading.percentage);
        Serial.println("%");
        xSemaphoreGive(xSerialMutex);
    }
}

void serialInputTask(void *parameter) {
    for (;;) {
        if (Serial.available() > 0) {
            char input = Serial.read();
            if (input == '1') {
                xEventGroupSetBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
            } else if (input == '2') {
                xEventGroupClearBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}