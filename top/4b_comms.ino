// ================= STATE =================
// Tracks whether sensor streaming is active
unsigned long long scheduledStartAt = 0;
// Timing for periodic sending
unsigned long lastSendTime = 0;
// Interval between sensor packets (milliseconds) — 50Hz
const unsigned long sendIntervalMs = 20;
// Statistics
unsigned long bytesSent = 0;

// ================= NTP SYNC =================
void syncNTP() {
    configTime(0, 0, ntp_server);
    xSemaphoreTake(xSerialMutex, portMAX_DELAY);
    Serial.print("Syncing NTP time");
    xSemaphoreGive(xSerialMutex);
    struct tm timeinfo;
    int ntp_retries = 0;
    unsigned long start = millis();
    while (!getLocalTime(&timeinfo) || timeinfo.tm_year < 100) {
        if (millis() - start > NTP_TIMEOUT_MS) {
            ntp_retries++;
            xSemaphoreTake(xSerialMutex, portMAX_DELAY);
            Serial.printf("\nNTP sync timed out (attempt %d) — retrying\n", ntp_retries);
            xSemaphoreGive(xSerialMutex);
            configTime(0, 0, ntp_server);
            start = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);
        Serial.print(".");
        xSemaphoreGive(xSerialMutex);
    }
    xSemaphoreTake(xSerialMutex, portMAX_DELAY);
    Serial.println("\nNTP time synced");
    xSemaphoreGive(xSerialMutex);
}

// Returns current Unix timestamp in milliseconds
unsigned long long getTimestampMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (unsigned long long)tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
}

// ================= WIFI CONNECTION =================
void connectWiFi() {
    xEventGroupClearBits(xSystemEventGroup, COMMS_FLAG_BIT | COMMS_RUNNING_FLAG_BIT);
    WiFi.disconnect(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    WiFi.begin(ssid, password);
#if defined(DEBUG)
    xSemaphoreTake(xSerialMutex, portMAX_DELAY);
    Serial.println("Connecting to WiFi");
    xSemaphoreGive(xSerialMutex);
#endif
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));

        xSemaphoreTake(xSerialMutex, portMAX_DELAY);
        Serial.print(".");
        xSemaphoreGive(xSerialMutex);
        attempts++;

        if (attempts >= WIFI_RETRY_ATTEMPTS) {
            xSemaphoreTake(xSerialMutex, portMAX_DELAY);
            Serial.println("\nWiFi connect failed — retrying");
            xSemaphoreGive(xSerialMutex);

            WiFi.disconnect(true);
            vTaskDelay(pdMS_TO_TICKS(100));

            WiFi.begin(ssid, password);
            attempts = 0;
        }
    }

#if defined(DEBUG)
    xSemaphoreTake(xSerialMutex, portMAX_DELAY);
    Serial.println("\nWiFi connected");
    xSemaphoreGive(xSerialMutex);
#endif

    syncNTP();
    xEventGroupSetBits(xSystemEventGroup, COMMS_FLAG_BIT);
}

// ================= MQTT CONNECTION =================
void connectMQTT() {
    xEventGroupClearBits(xSystemEventGroup, COMMS_FLAG_BIT | COMMS_RUNNING_FLAG_BIT);
    // Create unique client ID
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "%s_%s", player_id, node_id);
    // Keep attempting to connect until successful
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect(clientId)) {
            Serial.println("connected");
            // Subscribe to commands from server
            client.subscribe(commandTopic.c_str());   // command/player/node
            client.subscribe(broadcastTopic.c_str()); // command/player/all
            client.subscribe(inferenceTopic.c_str()); // inference/player/node

            // Notify laptop this node has (re)connected — laptop will re-send
            // current state
            client.publish(statusTopic.c_str(), "{\"event\":\"connected\"}",
                           false);
            Serial.println(
                "Status published — waiting for laptop to re-send state");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    xEventGroupSetBits(xSystemEventGroup, COMMS_FLAG_BIT);
}

// ================= SENSOR DATA PACKET =================
void sendSensorPacket(imu_reading_t *imu_data) {
    // Create JSON document
    StaticJsonDocument<512> doc;
    unsigned long long ts = getTimestampMs();
    doc["sequence"] = (uint64_t)ts;

    // ================= IMU 1 =================
    JsonObject imu1 = doc.createNestedObject(part1);
    JsonArray accel1 = imu1.createNestedArray("accel");
    accel1.add(imu_data[0].x);
    accel1.add(imu_data[0].y);
    accel1.add(imu_data[0].z);
    JsonArray gyro1 = imu1.createNestedArray("gyro");
    gyro1.add(imu_data[0].roll);
    gyro1.add(imu_data[0].pitch);
    gyro1.add(imu_data[0].yaw);

    // ================= IMU 2 =================
    JsonObject imu2 = doc.createNestedObject(part2);
    JsonArray accel2 = imu2.createNestedArray("accel");
    accel2.add(imu_data[1].x);
    accel2.add(imu_data[1].y);
    accel2.add(imu_data[1].z);
    JsonArray gyro2 = imu2.createNestedArray("gyro");
    gyro2.add(imu_data[1].roll);
    gyro2.add(imu_data[1].pitch);
    gyro2.add(imu_data[1].yaw);

    // Serialize JSON to buffer
    char buffer[512];
    size_t len = serializeJson(doc, buffer);
    // Publish JSON to sensor topic
    client.publish(sensorTopic.c_str(), buffer, len);
    // Update statistics
    bytesSent += len;

#if defined(DEBUG)
    Serial.printf("Sensor packet sent, seq=%llu, %d bytes\n", ts, len);
#endif
}

// ================= MQTT CALLBACK =================
void callback(char *topic, byte *payload, unsigned int length) {
    String topicStr = String(topic);
    StaticJsonDocument<256> incoming;
    DeserializationError err = deserializeJson(incoming, payload, length);
    if (err) {
#if defined(DEBUG)
        Serial.printf("JSON parse error: %s\n", err.c_str());
#endif
        return;
    }

    // ================= INFERENCE =================
    // Handle inference first — it has no "command" key
    if (topicStr == inferenceTopic) {
        int seq = incoming["seq"];
        Serial.printf("\n[INFERENCE RECEIVED] seq=%d\n", seq);

        JsonObject obj = incoming.as<JsonObject>();
        for (JsonPair kv : obj) {
            const char *key = kv.key().c_str();
            if (strcmp(key, "seq") == 0)
                continue;
            float score = kv.value().as<float>();
            Serial.printf("  %s -> %.3f\n", key, score);
        }

        // Check joint scores for haptic feedback
        float score1 = incoming[part1] | 1.0f;
        float score2 = incoming[part2] | 1.0f;

        if (score1 < HAPTIC_SCORE_THRESHOLD || score2 < HAPTIC_SCORE_THRESHOLD) {
            xEventGroupSetBits(xSystemEventGroup, HAPTICS_ON_BIT);
        } else {
            xEventGroupClearBits(xSystemEventGroup, HAPTICS_ON_BIT);
        }

        return;
    }

    // ================= COMMANDS =================
    // All other topics must have a "command" key
    const char *cmd = incoming["command"];
    if (!cmd)
        return;

    // Handle commands
    // Start Command
    if (strcmp(cmd, "start") == 0) {
        unsigned long long start_at = incoming["start_at"] | 0ULL;
        // Wait for scheduled start time if provided
        if (start_at > 0) {
            scheduledStartAt = start_at;
            xEventGroupClearBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
#if defined(DEBUG)
            Serial.printf("START scheduled at %llu (in ~%llums)\n", start_at,
                          start_at - getTimestampMs());
#endif
        }
        // No timestamp — start immediately (fallback)
        else {
            xEventGroupSetBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
            // Set calibration bits to 1 to trigger calibration
            xEventGroupSetBits(xSystemEventGroup, IMU_CALIB_FLAG_BITS);
            scheduledStartAt = 0;
#if defined(DEBUG)
            Serial.println("START command received (immediate)");
#endif
        }
    }
    // Stop command
    else if (strcmp(cmd, "stop") == 0) {
        xEventGroupClearBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT | HAPTICS_ON_BIT);
        scheduledStartAt = 0;
#if defined(DEBUG)
        Serial.println("STOP command received");
#endif
    }
}

void comms_init() {
    connectWiFi();
    // Set TLS certificate for MQTT
    espClient.setCACert(ca_cert);
    // Configure MQTT broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setBufferSize(512);
    // Set callback for incoming messages
    client.setCallback(callback);
    // Connect to MQTT
    connectMQTT();
}

/**
 * RTOS TASKS
 */
void commsSensorsTask(void *parameter) {
    // Brief delay to allow hardware to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    comms_init();

    imu_reading_t sensor_readings[NUM_IMU] = {0};
    EventBits_t expected_imu_bits;
    uint32_t received_imu_mask;

    // Wait until at least any IMU is initialized and comms is ready
    xEventGroupWaitBits(xSystemEventGroup,
                        IMU_FLAG_BITS | COMMS_FLAG_BIT,
                        pdFALSE,       // Do not clear bits on exit
                        pdFALSE,       // Any bit
                        portMAX_DELAY  // Wait indefinitely
    );

#if defined(DEBUG)
    Serial.println("Wireless Sensors Task Started");
    Serial.print("User: ");
    Serial.print(player_id);
    Serial.print(", Node: ");
    Serial.println(node_id);
#endif

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
#if defined(DEBUG)
            Serial.println("WiFi lost. Reconnecting...");
#endif
            connectWiFi();
        }

        // Reconnect MQTT if needed
        if (!client.connected()) {
            connectMQTT();
        }

        client.loop();

        // Check if isRunning bit is set
        if ((xEventGroupGetBits(xSystemEventGroup) & COMMS_RUNNING_FLAG_BIT) ==
            0) {
            // If not running, check if a scheduled start time has been reached
            if (scheduledStartAt > 0 && getTimestampMs() >= scheduledStartAt) {
                xEventGroupSetBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
                // Trigger recalibration on scheduled start (same as immediate start)
                xEventGroupSetBits(xSystemEventGroup, IMU_CALIB_FLAG_BITS);
                scheduledStartAt = 0;
#if defined(DEBUG)
                Serial.println("Scheduled start triggered");
#endif
            }
            // Sleep briefly to avoid busy loop when not running
            // We also skip sending if not running
            else {
                vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_DELAY_MS));
                continue;
            }
        }

        // Wait for new data from all initialized IMUs
        expected_imu_bits =
            xEventGroupGetBits(xSystemEventGroup) & IMU_FLAG_BITS;
        received_imu_mask = 0;

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            // Check if IMU is initialized (bit is 1)
            if (expected_imu_bits & (1 << sensor_id)) {
                // Try to pull data from its queue
                if (xQueueReceive(xIMUQueue[sensor_id],
                                  &sensor_readings[sensor_id], 0) == pdTRUE) {
                    // Set this IMU bit in our local tracker
                    received_imu_mask |= (1 << sensor_id);
                }
            }
            // If not initialised
            else {
                memset(&sensor_readings[sensor_id], 0,
                       sizeof(sensor_readings[sensor_id]));
            }
        }

        // Send data if all initialized IMUs have new data
        if (expected_imu_bits > 0 && received_imu_mask == expected_imu_bits) {
            sendSensorPacket(sensor_readings);
        }

        // Sleep briefly to yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_DELAY_MS));
    }
}

void commsBattTask(void *parameter) {}