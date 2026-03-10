#if defined(DEBUG)
void monitorTask(void *pvParameters) {
    for (;;) {
        // 1. Get the number of tasks currently running
        UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
        TaskStatus_t *pxTaskStatusArray =
            (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

        if (pxTaskStatusArray != NULL) {
            // 2. Generate the snapshot
            uxArraySize =
                uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

            Serial.println("\n--- Task Monitor ---");
            Serial.printf("%-16s %-10s %-10s %-10s\n", "Name", "State",
                          "Priority", "StackLeft");

            for (UBaseType_t i = 0; i < uxArraySize; i++) {
                const char *state;
                switch (pxTaskStatusArray[i].eCurrentState) {
                case eRunning:
                    state = "Running";
                    break;
                case eReady:
                    state = "Ready";
                    break;
                case eBlocked:
                    state = "Blocked";
                    break;
                case eSuspended:
                    state = "Suspended";
                    break;
                case eDeleted:
                    state = "Deleted";
                    break;
                default:
                    state = "Unknown";
                    break;
                }

                Serial.printf(
                    "%-16s %-10s %-10d %-10u\n",
                    pxTaskStatusArray[i].pcTaskName, state,
                    (int)pxTaskStatusArray[i].uxCurrentPriority,
                    (unsigned int)pxTaskStatusArray[i].usStackHighWaterMark);
            }
            Serial.println();

            // 3. Free the memory used for the snapshot
            vPortFree(pxTaskStatusArray);
        }

        // Run every 5 seconds to avoid flooding the Serial port
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
#endif