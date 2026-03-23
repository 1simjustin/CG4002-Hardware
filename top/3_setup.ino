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

    // If WiFi Comms is disabled, set COMMS_RUNNING_FLAG_BIT immediately to allow system to run
#if !defined(ENABLE_WIFI_COMMS)
    xEventGroupSetBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
#endif
    
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

    // Battery Semaphore
    xBattSemaphore = xSemaphoreCreateBinary();
#if defined(DEBUG)
    if (xBattSemaphore == NULL) {
        Serial.println("Failed to create Battery semaphore");
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