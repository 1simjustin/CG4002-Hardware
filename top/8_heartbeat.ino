void hbDispTask(void *parameter) {
    pinMode(HB_LED_PIN, OUTPUT);

    for (;;) {
        // check if imu_init is all true, if not, blink at 1Hz to indicate waiting for IMU initialization
        bool all_imu_init = true;
        for (int i = 0; i < NUM_IMU; i++) {
            if (!imu_init[i]) {
                all_imu_init = false;
                break;
            }
        }

        if (!all_imu_init) {
            blink_state_hb = !blink_state_hb;
            digitalWrite(HB_LED_PIN, blink_state_hb ? HIGH : LOW);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        } else {
            // If all IMUs are initialized, turn on the LED solid
            digitalWrite(HB_LED_PIN, HIGH);
            vTaskDelete(NULL);
        }
    }
}