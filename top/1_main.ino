#include "comms_header.h"
#include "definitions.h"
#include "header.h"

void setup() {
    // Set CPU frequency for power saving (idle until first start command)
    setCpuFrequencyMhz(CPU_FREQ_IDLE_MHZ);

    Serial.begin(115200);

    // Initialize I2C
    i2c_setup();

    BaseType_t result;

    // Initialize Semaphores
    createSemaphores();

    delay(10);

    // Create RTOS Tasks
    // Heartbeat Display Task
    result = xTaskCreatePinnedToCore(hbDispTask,             // Task function
                                     "HeartbeatDisplayTask", // Task name
                                     COMMS_STACK_SIZE, // Stack size (bytes)
                                     NULL,             // Parameters
                                     ACTUATOR_TASK_PRIORITY, // Priority
                                     &hbDispTaskHandle,      // Task handle
                                     SENSOR_CORE // Core 1 for sensors
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: HeartbeatDisplayTask");
    }

    // IMU Tasks
    for (int i = 0; i < NUM_IMU; i++) {
        char imuTaskName[20];
        sprintf(imuTaskName, "IMUTask%d", i);
        result =
            xTaskCreatePinnedToCore(imuTask,        // Task function
                                    imuTaskName,    // Task name
                                    STACK_SIZE, // Stack size (bytes)
                                    (void *)i, // Parameters (pass IMU index)
                                    IMU_TASK_PRIORITY, // Priority
                                    &IMUTaskHandle[i], // Task handle
                                    SENSOR_CORE        // Core 1 for sensors
            );
        if (result != pdPASS) {
            Serial.print("Failed to create task: ");
            Serial.println(imuTaskName);
        }
    }

    // Comms Tasks
#if defined(ENABLE_SENSOR_COMMS)
#if defined(ENABLE_WIFI_COMMS)
    // Watchdog task — monitors WiFi/MQTT connectivity
    result = xTaskCreatePinnedToCore(commsWatchdogTask,    // Task function
                                     "CommsWatchdogTask",  // Task name
                                     COMMS_STACK_SIZE,     // Stack size (bytes)
                                     NULL,                 // Parameters
                                     COMMS_WDG_TASK_PRIORITY, // Priority
                                     &CommsWatchdogTaskHandle,    // Task handle
                                     COMMS_CORE // Core 0 for comms
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: CommsWatchdogTask");
    }

    // Sensor data task — sends IMU data over MQTT
    result = xTaskCreatePinnedToCore(commsSensorsTask,   // Task function
                                     "CommsSensorsTask", // Task name
                                     COMMS_STACK_SIZE,   // Stack size (bytes)
                                     NULL,               // Parameters
                                     SENSORS_COMMS_TASK_PRIORITY, // Priority
                                     &CommsSensorsTaskHandle,     // Task handle
                                     COMMS_CORE // Core 0 for comms
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: CommsSensorsTask");
    }
#else
    result = xTaskCreatePinnedToCore(serialSensorsTask,  // Task function
                                     "CommsSensorsTask", // Task name
                                     STACK_SIZE,         // Stack size (bytes)
                                     NULL,               // Parameters
                                     SENSORS_COMMS_TASK_PRIORITY, // Priority
                                     &CommsSensorsTaskHandle,     // Task handle
                                     COMMS_CORE // Core 0 for comms
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: CommsSensorsTask");
    }

    result = xTaskCreatePinnedToCore(serialInputTask,         // Task function
                                     "SerialInputTask",       // Task name
                                     STACK_SIZE,              // Stack size (bytes)
                                     NULL,                    // Parameters
                                     BATT_COMMS_TASK_PRIORITY, // Priority
                                     &SerialInputTaskHandle,  // Task handle
                                     COMMS_CORE               // Core 0 for comms
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: SerialInputTask");
    }
#endif
#endif

    // Battery Reader Tasks
    result = xTaskCreatePinnedToCore(battTask,           // Task function
                                     "BatteryTask",      // Task name
                                     STACK_SIZE,         // Stack size (bytes)
                                     NULL,               // Parameters
                                     BATT_TASK_PRIORITY, // Priority
                                     &BatteryTaskHandle, // Task handle
                                     SENSOR_CORE         // Core 1 for sensors
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: BatteryTask");
    }

    // Battery Comms Task
    result = xTaskCreatePinnedToCore(serialBattTask,   // Task function
                                     "SerialBattTask", // Task name
                                     STACK_SIZE,       // Stack size (bytes)
                                     NULL,             // Parameters
                                     BATT_COMMS_TASK_PRIORITY, // Priority
                                     &CommsBattTaskHandle,     // Task handle
                                     COMMS_CORE // Core 0 for comms
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: SerialBattTask");
    }

    // Battery Display Task
    result = xTaskCreatePinnedToCore(battDispTask,         // Task function
                                     "BatteryDisplayTask", // Task name
                                     STACK_SIZE,           // Stack size (bytes)
                                     NULL,                 // Parameters
                                     ACTUATOR_TASK_PRIORITY, // Priority
                                     &BattDispTaskHandle,    // Task handle
                                     SENSOR_CORE             // Core 1 for sensors
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: BatteryDisplayTask");
    }

    // Haptic Task
    // Only if PLAYERS is defined as "student" and ENABLE_HAPTICS is defined,
    // since only student node has haptics
#if defined(ENABLE_HAPTICS) && (PLAYER == PLAYER_STUDENT)
    result = xTaskCreatePinnedToCore(hapticTask,           // Task function
                                     "HapticTask",         // Task name
                                     COMMS_STACK_SIZE,     // Stack size (bytes)
                                     NULL,                 // Parameters
                                     HAPTIC_TASK_PRIORITY, // Priority
                                     &hapticTaskHandle,    // Task handle
                                     SENSOR_CORE           // Core 1 for sensors
    );
    if (result != pdPASS) {
        Serial.println("Failed to create task: HapticTask");
    }
#endif

    // Monitor Task (Debug Only)
    // #if defined(DEBUG)
    //     result = xTaskCreatePinnedToCore(monitorTask,        // Task function
    //                                      "MonitorTask",      // Task name
    //                                      STACK_SIZE,         // Stack size (bytes)
    //                                      NULL,               // Parameters
    //                                      1,                  // Priority (lowest)
    //                                      &MonitorTaskHandle, // Task handle
    //                                      COMMS_CORE          // Core 0 for comms
    //     );
    //     if (result != pdPASS) {
    //         Serial.println("Failed to create task: MonitorTask");
    //     }
    // #endif
}

void loop() {
    // Empty because FreeRTOS scheduler runs the task
}