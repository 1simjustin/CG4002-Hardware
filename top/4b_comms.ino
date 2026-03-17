// ================= STATE =================
// Tracks whether sensor streaming is active
bool isRunning = false;
unsigned long long scheduledStartAt = 0;
// Timing for periodic sending
unsigned long lastSendTime = 0;
// Interval between sensor packets (milliseconds) — 50Hz
const unsigned long sendIntervalMs = 20;
// Statistics
unsigned long bytesSent = 0;

void syncNTP() {
    configTime(0, 0, ntp_server);
    Serial.print("Syncing NTP time");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo) || timeinfo.tm_year < 100) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nNTP time synced");
}

// Returns current Unix timestamp in milliseconds
unsigned long long getTimestampMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (unsigned long long)tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
}

// ================= WIFI CONNECTION =================
void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    syncNTP();
}

// ================= MQTT CONNECTION =================
void connectMQTT() {
    // Create unique client ID
    String clientId = String(player_id) + "_" + String(node_id);
    // Keep attempting to connect until successful
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            // Subscribe to commands from server
            client.subscribe(commandTopic.c_str());   // command/player/node
            client.subscribe(broadcastTopic.c_str()); // command/player/all

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
            delay(1000);
        }
    }
}

void sendSensorPacket(imu_reading_t *imu_data) {
    // Create JSON document
    StaticJsonDocument<512> doc;
    unsigned long long ts = getTimestampMs();
    doc["sequence"] = (uint64_t)ts;
    // Decide body part names automatically
    String part1;
    String part2;
    if (String(node_id).indexOf("arm") >= 0) {
        part1 = "_upper_arm";
        part2 = "_forearm";
    } else {
        part1 = "_thigh";
        part2 = "_shin";
    }

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

void callback(char *topic, byte *payload, unsigned int length) {
    StaticJsonDocument<128> incoming;
    DeserializationError err = deserializeJson(incoming, payload, length);
    if (err)
        return;
    const char *cmd = incoming["command"];
    if (!cmd)
        return;
    // Handle commands
    if (strcmp(cmd, "start") == 0) {
        unsigned long long start_at = incoming["start_at"] | 0ULL;
        if (start_at > 0) {
            scheduledStartAt = start_at;
            isRunning = false;
            Serial.printf("START scheduled at %llu (in ~%llums)\n", start_at,
                          start_at - getTimestampMs());
        } else {
            // No timestamp — start immediately (fallback)
            isRunning = true;
            scheduledStartAt = 0;
            Serial.println("START command received (immediate)");
        }
    } else if (strcmp(cmd, "stop") == 0) {
        isRunning = false;
        scheduledStartAt = 0;
        Serial.println("STOP command received");
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
    bool any_imu_ready;

    // Wait until IMUs are initialized before starting to send data
    do {
        any_imu_ready = false;
        for (int i = 0; i < NUM_IMU; i++) {
            if (imu_init[i]) {
                any_imu_ready = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    } while (!any_imu_ready);

#if defined(DEBUG)
    Serial.println("Serial Sensors Task Started");
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

        if (!isRunning) {
            if (scheduledStartAt > 0 && getTimestampMs() >= scheduledStartAt) {
                isRunning = true;
                scheduledStartAt = 0;
#if defined(DEBUG)
                Serial.println("Scheduled start triggered");
#endif
            } else {
                // Sleep briefly to avoid busy loop
                vTaskDelay(pdMS_TO_TICKS(100));
                // Skip sending if not running
                continue;
            }
        }

        // Wait for new data from all initialized IMUs
        bool data_rdy = true;
        for (int i = 0; i < NUM_IMU; i++) {
            if (!imu_init[i]) {
                continue; // Skip if IMU failed to initialize
            }
            if (xQueueReceive(xIMUQueue[i], &sensor_readings[i], 0) != pdTRUE) {
                data_rdy = false;
                break;
            }
        }

        if (data_rdy) {
            sendSensorPacket(sensor_readings);
        }
    }
}

void commsBattTask(void *parameter) {}