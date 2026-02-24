void i2c_setup() {
    Wire.begin();
    Wire.setClock(400000);
}

void createSemaphores() {
    #if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
    for (int i = 0; i < NUM_IMUS; i++) {
        xIMUSemaphore[i] = xSemaphoreCreateBinary();
    }
    for (int i = 0; i < NUM_FLEX; i++) {
        xFlexSemaphore[i] = xSemaphoreCreateBinary();
    }
    #endif

    xBattSemaphore = xSemaphoreCreateBinary();
}