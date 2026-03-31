void hbDispTask(void *parameter) {
    pinMode(HB_LED_PIN, OUTPUT);
    EventBits_t system_bits;

    for (;;) {
        system_bits = xEventGroupGetBits(xSystemEventGroup);
        // If any IMU is not initialized, blink at 1Hz to indicate waiting for IMU initialization
        if ((system_bits & IMU_FLAG_BITS) != IMU_FLAG_BITS) {
            blink_state_hb = !blink_state_hb;
            digitalWrite(HB_LED_PIN, blink_state_hb ? HIGH : LOW);
            vTaskDelay(pdMS_TO_TICKS(HB_INIT_BLINK_PERIOD_MS));
            continue;
        }

        // If all IMUs are initialized, blink at 2Hz to indicate waiting for IMU calibration
        if ((system_bits & IMU_CALIB_FLAG_BITS) != 0) {
            blink_state_hb = !blink_state_hb;
            digitalWrite(HB_LED_PIN, blink_state_hb ? HIGH : LOW);
            vTaskDelay(pdMS_TO_TICKS(HB_CALIB_BLINK_PERIOD_MS));
            continue;
        }

        // If all IMUs are initialized and waiting for wifi connection, blink at 4Hz
        if ((system_bits & COMMS_FLAG_BIT) == 0) {
            blink_state_hb = !blink_state_hb;
            digitalWrite(HB_LED_PIN, blink_state_hb ? HIGH : LOW);
            vTaskDelay(pdMS_TO_TICKS(HB_WIFI_BLINK_PERIOD_MS));
            continue;
        }

        // If system is running, set LED to solid on
        if ((system_bits & COMMS_RUNNING_FLAG_BIT) != 0) {
            digitalWrite(HB_LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // If system is not running, set the LED to breathing effect
        else {
            led_pwm += led_pwm_dir * 5;
            if (led_pwm == 0 || led_pwm == 255) {
                led_pwm_dir *= -1; // Reverse direction at bounds
            }
            analogWrite(HB_LED_PIN, led_pwm);
            vTaskDelay(pdMS_TO_TICKS(25));
        }
    }
}