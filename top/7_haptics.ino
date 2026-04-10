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

void hapticTask(void *parameter) {
    haptic_setup();

    for (;;) {
        // Block until HAPTICS_ON_BIT is set
        analogWrite(HAPTIC_PIN, HAPTIC_OFF_PWM);
        xEventGroupWaitBits(xSystemEventGroup, // Event group handle
                            HAPTIC_TRIG_BITS, // Bits to wait for
                            pdFALSE,       // Do not clear bits on exit
                            pdTRUE,        // Wait for ALL bits
                            portMAX_DELAY  // Wait indefinitely
        );

        // Pulse ON
        analogWrite(HAPTIC_PIN, HAPTIC_ON_PWM);
        vTaskDelay(pdMS_TO_TICKS(HAPTIC_PULSE_MS));

        // Re-check both bits before pausing — exit early if either cleared
        if ((xEventGroupGetBits(xSystemEventGroup) & HAPTIC_TRIG_BITS) == HAPTIC_TRIG_BITS) {
            analogWrite(HAPTIC_PIN, HAPTIC_OFF_PWM);
            vTaskDelay(pdMS_TO_TICKS(HAPTIC_PAUSE_MS));
        }
    }
}