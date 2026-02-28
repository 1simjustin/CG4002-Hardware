void i2c_setup() {
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);

    Wire.begin();
    Wire.setClock(400000);
}

void createSemaphores() {
    #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
    for (int i = 0; i < NUM_IMU; i++) {
        xIMUSemaphore[i] = xSemaphoreCreateBinary();
    }
    for (int i = 0; i < NUM_FLEX; i++) {
        xFlexSemaphore[i] = xSemaphoreCreateBinary();
    }
    #endif

    xBattSemaphore = xSemaphoreCreateBinary();
}