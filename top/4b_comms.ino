// ================= STATE =================
// Tracks whether sensor streaming is active
volatile unsigned long long scheduledStartAt = 0;
portMUX_TYPE scheduledStartMux = portMUX_INITIALIZER_UNLOCKED;
// Timing for periodic sending
unsigned long lastSendTime = 0;
// Interval between sensor packets (milliseconds) — 50Hz
const unsigned long sendIntervalMs = 20;
// Statistics
unsigned long bytesSent = 0;
// NTP state — false until first successful sync
bool ntpSynced = false;

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

            if (ntp_retries >= NTP_MAX_RETRIES) {
                ntpSynced = false;
                xEventGroupClearBits(xSystemEventGroup, NTP_SYNCED_BIT);
                xSemaphoreTake(xSerialMutex, portMAX_DELAY);
                Serial.println("\nNTP sync failed — scheduled starts will fall back to immediate");
                xSemaphoreGive(xSerialMutex);
                return;
            }

            xSemaphoreTake(xSerialMutex, portMAX_DELAY);
            Serial.printf("\nNTP sync timed out (attempt %d/%d) — retrying\n",
                          ntp_retries, NTP_MAX_RETRIES);
            xSemaphoreGive(xSerialMutex);

            configTime(0, 0, ntp_server);
            start = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);
        Serial.print(".");
        xSemaphoreGive(xSerialMutex);
    }

    ntpSynced = true;
    xEventGroupSetBits(xSystemEventGroup, NTP_SYNCED_BIT);
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

    // Enable WiFi power saving and reduce TX power
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    WiFi.setTxPower(WIFI_TX_POWER_DBM);

    if (!ntpSynced) {
        syncNTP();
    }
    xEventGroupSetBits(xSystemEventGroup, COMMS_FLAG_BIT);
}

// ================= MQTT CONNECTION =================
void connectMQTT() {
    // Create unique client ID
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "%s_%s", player_id, node_id);

    // Keep attempting to connect until successful
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect(clientId)) {
            Serial.println("connected");
            // Subscribe to commands from server
            client.subscribe(commandTopic);   // command/player/node
            client.subscribe(broadcastTopic); // command/player/all
            client.subscribe(inferenceTopic); // inference/player/node

            // Notify laptop this node has (re)connected — laptop will re-send
            // current state
            client.publish(statusTopic, "{\"event\":\"connected\"}", false);
            Serial.println(
                "Status published — waiting for laptop to re-send state");
        }
        // Failed to connect — wait and retry
        else {
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
    client.publish(sensorTopic, buffer, len);
    // Update statistics
    bytesSent += len;

#if defined(DEBUG)
    Serial.printf("Sensor packet sent, seq=%llu, %d bytes\n", ts, len);
#endif
}

// ================= MQTT CALLBACK =================
void callback(char *topic, byte *payload, unsigned int length) {
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
    if (strcmp(topic, inferenceTopic) == 0) {
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

        // Trigger haptics if either score is below threshold
        if (score1 < HAPTIC_SCORE_THRESHOLD ||
            score2 < HAPTIC_SCORE_THRESHOLD) {
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
        
        // Wait for scheduled start time if provided and NTP is available
        if (start_at > 0 && ntpSynced) {
            portENTER_CRITICAL(&scheduledStartMux);
            scheduledStartAt = start_at;
            portEXIT_CRITICAL(&scheduledStartMux);
            xEventGroupClearBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
#if defined(DEBUG)
            Serial.printf("START scheduled at %llu (in ~%llums)\n", start_at,
                          start_at - getTimestampMs());
#endif
        }
        // No timestamp — start immediately (fallback)
        else {
            // Switch to active power profile
            setCpuFrequencyMhz(CPU_FREQ_ACTIVE_MHZ);
            Wire.setClock(400000); // Reconfigure I2C divider for restored APB=80MHz
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

            xEventGroupSetBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
            // Set calibration bits to 1 to trigger calibration
            xEventGroupSetBits(xSystemEventGroup, IMU_CALIB_FLAG_BITS);
            portENTER_CRITICAL(&scheduledStartMux);
            scheduledStartAt = 0;
            portEXIT_CRITICAL(&scheduledStartMux);
#if defined(DEBUG)
            Serial.println("START command received (immediate)");
#endif
        }
    }
    // Stop command
    else if (strcmp(cmd, "stop") == 0) {
        xEventGroupClearBits(xSystemEventGroup,
                             COMMS_RUNNING_FLAG_BIT | HAPTICS_ON_BIT);
        portENTER_CRITICAL(&scheduledStartMux);
        scheduledStartAt = 0;
        portEXIT_CRITICAL(&scheduledStartMux);

        // Switch to idle power profile
        setCpuFrequencyMhz(CPU_FREQ_IDLE_MHZ);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

#if defined(DEBUG)
        Serial.println("STOP command received");
#endif
    }
}

void comms_init() {
    buildTopics();
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

// Watchdog task — monitors WiFi/MQTT connectivity and manages COMMS_FLAG_BIT
void commsWatchdogTask(void *parameter) {
    vTaskDelay(pdMS_TO_TICKS(100));
    comms_init();

    for (;;) {
        // Monitor WiFi
        if (WiFi.status() != WL_CONNECTED) {
            xEventGroupClearBits(xSystemEventGroup,
                                 COMMS_FLAG_BIT | COMMS_RUNNING_FLAG_BIT);
#if defined(DEBUG)
            Serial.println("WiFi lost. Reconnecting...");
#endif
            connectWiFi();
        }

        // Monitor MQTT
        if (!client.connected()) {
            xEventGroupClearBits(xSystemEventGroup,
                                 COMMS_FLAG_BIT | COMMS_RUNNING_FLAG_BIT);
            connectMQTT();
        }

        // Use longer delay when idle (not streaming)
        bool isRunning = (xEventGroupGetBits(xSystemEventGroup) & COMMS_RUNNING_FLAG_BIT) != 0;
        vTaskDelay(pdMS_TO_TICKS(isRunning ? COMMS_TASK_DELAY_MS : IDLE_WATCHDOG_POLL_MS));
    }
}

// Sensor data task — handles MQTT messages and IMU data publishing
void commsSensorsTask(void *parameter) {
    imu_reading_t sensor_readings[NUM_IMU] = {0};
    EventBits_t expected_imu_bits;
    uint32_t received_imu_mask;

    // Wait until at least any IMU is initialized and comms is ready
    xEventGroupWaitBits(xSystemEventGroup, IMU_FLAG_BITS | COMMS_FLAG_BIT,
                        pdFALSE,      // Do not clear bits on exit
                        pdFALSE,      // Any bit
                        portMAX_DELAY // Wait indefinitely
    );

#if defined(DEBUG)
    Serial.println("Wireless Sensors Task Started");
    Serial.print("User: ");
    Serial.print(player_id);
    Serial.print(", Node: ");
    Serial.println(node_id);
#endif

    for (;;) {
        // Block until comms is available
        if ((xEventGroupGetBits(xSystemEventGroup) & COMMS_FLAG_BIT) == 0) {
            xEventGroupWaitBits(xSystemEventGroup, COMMS_FLAG_BIT,
                                pdFALSE, pdTRUE, portMAX_DELAY);
        }

        client.loop();

        // Check if isRunning bit is set
        if ((xEventGroupGetBits(xSystemEventGroup) & COMMS_RUNNING_FLAG_BIT) ==
            0) {
            // If not running, check if a scheduled start time has been reached
            portENTER_CRITICAL(&scheduledStartMux);
            unsigned long long startAt = scheduledStartAt;
            portEXIT_CRITICAL(&scheduledStartMux);
            if (startAt > 0 && getTimestampMs() >= startAt) {
                // Switch to active power profile
                setCpuFrequencyMhz(CPU_FREQ_ACTIVE_MHZ);
                Wire.setClock(400000); // Reconfigure I2C divider for restored APB=80MHz
                esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

                xEventGroupSetBits(xSystemEventGroup, COMMS_RUNNING_FLAG_BIT);
                xEventGroupSetBits(xSystemEventGroup, IMU_CALIB_FLAG_BITS);
                portENTER_CRITICAL(&scheduledStartMux);
                scheduledStartAt = 0;
                portEXIT_CRITICAL(&scheduledStartMux);
#if defined(DEBUG)
                Serial.println("Scheduled start triggered");
#endif
            } else {
                vTaskDelay(pdMS_TO_TICKS(IDLE_COMMS_POLL_MS));
                continue;
            }
        }

        // Wait for new data from all initialized IMUs
        expected_imu_bits =
            xEventGroupGetBits(xSystemEventGroup) & IMU_FLAG_BITS;
        received_imu_mask = 0;

        for (int sensor_id = 0; sensor_id < NUM_IMU; sensor_id++) {
            if (expected_imu_bits & (1 << sensor_id)) {
                if (xQueueReceive(xIMUQueue[sensor_id],
                                  &sensor_readings[sensor_id], 0) == pdTRUE) {
                    received_imu_mask |= (1 << sensor_id);
                }
            } else {
                memset(&sensor_readings[sensor_id], 0,
                       sizeof(sensor_readings[sensor_id]));
            }
        }

        // Send if all initialized IMUs have data and comms still connected
        if (expected_imu_bits > 0 && received_imu_mask == expected_imu_bits &&
            (xEventGroupGetBits(xSystemEventGroup) & COMMS_FLAG_BIT)) {
            sendSensorPacket(sensor_readings);
        }

        vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_DELAY_MS));
    }
}

void commsBattTask(void *parameter) {}