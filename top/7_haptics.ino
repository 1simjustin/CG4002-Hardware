#include "definitions.h"
#include "header.h"

void haptic_setup() {
    pinMode(HAPTIC_PIN, OUTPUT);

    if (!drv.begin()) {
#if defined(DEBUG)
        Serial.println("Could not find DRV2605");
#endif
        vTaskDelete(NULL);
    }
    
    drv.selectLibrary(1);
    drv.setMode(DRV2605_MODE_PWMANALOG);
}

void hapticToggle() {
    if (haptic_state) {
        analogWrite(HAPTIC_PIN, HAPTIC_ON_PWM);
        vTaskDelay(pdMS_TO_TICKS(HAPTIC_PULSE_MS));
    } else {
        analogWrite(HAPTIC_PIN, HAPTIC_OFF_PWM);
        vTaskDelay(pdMS_TO_TICKS(HAPTIC_PAUSE_MS));
    }
    haptic_state = !haptic_state;
}

void hapticOff() {
    analogWrite(HAPTIC_PIN, HAPTIC_OFF_PWM);
}

void hapticTask(void *parameter) {
    haptic_setup();

    for (;;) {
        // Turn off haptics before waiting for next trigger to avoid unwanted
        // vibrations
        hapticOff();

        // Check if HAPTICS_ON_BIT is set, if not, sleep until it is set
        xEventGroupWaitBits(xSystemEventGroup, // Event group
                            HAPTICS_ON_BIT,    // Bits to wait for
                            pdFALSE,           // Do not clear bits on exit
                            pdTRUE,            // Wait for all bits
                            portMAX_DELAY      // Wait indefinitely
        );

        // Toggle haptics while HAPTICS_ON_BIT is set
        // hapticOff should not interfere with the timing of hapticToggle since
        // it is only called once before waiting for the next trigger, and the
        // toggle timing is determined by HAPTIC_PULSE_MS and HAPTIC_PAUSE_MS
        hapticToggle();
    }
}