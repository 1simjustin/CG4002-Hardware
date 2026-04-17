void i2c_setup() {
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);

    Wire.begin();
    Wire.setClock(400000);
}

void createSemaphores() {
    // IMU Event Group
    // Null checks not needed for static event groups as they are statically allocated
    xSystemEventGroup = xEventGroupCreateStatic(&xSystemEventGroupBuffer);
    
    // IMU Data Queues
    for (int i = 0; i < NUM_IMU; i++) {
        xIMUQueue[i] = xQueueCreate(1, sizeof(imu_reading_t));
#if defined(DEBUG)
        if (xIMUQueue[i] == NULL) {
            Serial.print("Failed to create IMU queue for IMU ");
            Serial.println(i);
        }
#endif
    }

    // Battery Queues
    xBattDispQueue = xQueueCreate(1, sizeof(batt_reading_t));
    xBattSerialQueue = xQueueCreate(1, sizeof(batt_reading_t));
#if defined(DEBUG)
    if (xBattDispQueue == NULL) {
        Serial.println("Failed to create Battery display queue");
    }
    if (xBattSerialQueue == NULL) {
        Serial.println("Failed to create Battery serial queue");
    }
#endif

    // Serial Mutex
    xSerialMutex = xSemaphoreCreateMutex();
#if defined(DEBUG)
    if (xSerialMutex == NULL) {
        Serial.println("Failed to create Serial mutex");
    }
#endif
}