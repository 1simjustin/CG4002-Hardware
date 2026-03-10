void i2c_setup() {
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);

    Wire.begin();
    Wire.setClock(400000);
}

void createSemaphores() {
    for (int i = 0; i < NUM_IMU; i++) {
        xIMUSemaphore[i] = xSemaphoreCreateBinary();
#if defined(DEBUG)
        if (xIMUSemaphore[i] == NULL) {
            Serial.print("Failed to create IMU semaphore for IMU ");
            Serial.println(i);
        }
#endif
    }

    xBattSemaphore = xSemaphoreCreateBinary();
#if defined(DEBUG)
    if (xBattSemaphore == NULL) {
        Serial.println("Failed to create Battery semaphore");
    }
#endif

    xSerialMutex = xSemaphoreCreateMutex();
#if defined(DEBUG)
    if (xSerialMutex == NULL) {
        Serial.println("Failed to create Serial mutex");
    }
#endif
}