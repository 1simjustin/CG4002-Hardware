void hbDispTask(void *parameter) {
    pinMode(HB_LED_PIN, OUTPUT);
    EventBits_t imu_init_bits;

    for (;;) {
        imu_init_bits = xEventGroupGetBits(xIMUEventGroup);
        // If any IMU is not initialized, blink at 1Hz to indicate waiting for IMU initialization
        if ((imu_init_bits & IMU_FLAG_BITS) != IMU_FLAG_BITS) {
            blink_state_hb = !blink_state_hb;
            digitalWrite(HB_LED_PIN, blink_state_hb ? HIGH : LOW);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            // If all IMUs are initialized, turn on the LED solid
            digitalWrite(HB_LED_PIN, HIGH);
            vTaskDelete(NULL);
        }
    }
}