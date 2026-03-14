// void connectWiFi() {
//     WiFi.begin(ssid, password);
//     Serial.print("Connecting to WiFi");
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("\nWiFi connected");
// }

// void connectMQTT() {
//     // Create unique client ID
//     String clientId = String(player_id) + "_" + String(node_id);
//     // Keep attempting to connect until successful
//     while (!client.connected()) {
//         Serial.print("Connecting to MQTT...");
//         if (client.connect(clientId.c_str())) {
//             Serial.println("connected");
//             // Subscribe to commands from server
//             client.subscribe(commandTopic.c_str());
//         } else {
//             Serial.print("failed, rc=");
//             Serial.print(client.state());
//             Serial.println(" retrying...");
//             delay(1000);
//         }
//     }
// }

// void sendSensorPacket(imu_reading_t (*imu_data)[NUM_IMU]) {
//     // Create JSON document
//     StaticJsonDocument<512> doc;
//     // Sequence number and timestamp
//     doc["sequence"] = sequenceCounter++;
//     doc["timestamp_ms"] = millis();
//     // Decide body part names automatically
//     String part1;
//     String part2;
//     if (String(node_id).indexOf("arm") >= 0) {
//         part1 = "upper_arm";
//         part2 = "forearm";
//     } else {
//         part1 = "thigh";
//         part2 = "shin";
//     }
//     // Add IMU data to JSON
//     for (int i = 0; i < NUM_IMU; i++) {
//         JsonObject imuObj = doc["imus"][i];
//         imuObj["id"] = i;
//         imuObj["acceleration"] = {{"x", imu_data[i]->x},
//                                   {"y", imu_data[i]->y},
//                                   {"z", imu_data[i]->z}};
//         imuObj["gyro"] = {{"roll", imu_data[i]->roll},
//                           {"pitch", imu_data[i]->pitch},
//                           {"yaw", imu_data[i]->yaw}};
//     }
//     // Serialize JSON to buffer
//     char buffer[512];
//     size_t len = serializeJson(doc, buffer);
//     // Publish to MQTT
//     client.publish(sensorTopic.c_str(), buffer, len);
//     // Update statistics
//     bytesSent += len;

//     Serial.printf("Sensor packet sent, %d bytes\n", len);
// }

// void comms_init() {
//     connectWiFi();
//     // Set TLS certificate for MQTT
//     espClient.setCACert(ca_cert);
//     // Configure MQTT broker
//     client.setServer(mqtt_broker, mqtt_port);
//     // Set callback for incoming messages
//     client.setCallback(callback);
//     // Connect to MQTT
//     connectMQTT();
// }

// void commsSensorsTask(void *parameter) {
//     comms_init();

//     imu_reading_t sensor_readings[NUM_IMU] = {0};
//     bool any_imu_ready = false;

//     // Wait until IMUs are initialized before starting to send data
//     while (!any_imu_ready) {
//         bool any_imu_init = false;
//         for (int i = 0; i < NUM_IMU; i++) {
//             if (imu_init[i]) {
//                 any_imu_ready = true;
//                 break;
//             }
//         }
//         if (any_imu_init) {
//             break;
//         }
//         vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
//     }

// #if defined(DEBUG)
//     Serial.println("Serial Sensors Task Started");
// #endif

    
//     for (;;) {
//         if (WiFi.status() != WL_CONNECTED) {
//             Serial.println("WiFi lost. Reconnecting...");
//             connectWiFi();
//         }

//         // Reconnect MQTT if needed
//         if (!client.connected()) {
//             connectMQTT();
//         }

//         client.loop();

//         if (!isRunning) {
//             continue; // Skip sending if not running
//         }

//         // Wait for new data from all initialized IMUs
//         bool data_rdy = true;
//         for (int i = 0; i < NUM_IMU; i++) {
//             if (!imu_init[i]) {
//                 continue; // Skip if IMU failed to initialize
//             }
//             if (xQueueReceive(xIMUQueue[i], &sensor_readings[i], 0) != pdTRUE) {
//                 data_rdy = false;
//                 break;
//             }
//         }

//         if (data_rdy) {
//             sendSensorPacket(&sensor_readings);
//         }
//     }
// }

// void commsBattTask(void *parameter) {}