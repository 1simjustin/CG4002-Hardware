#ifndef COMMS_HEADER_H
#define COMMS_HEADER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ================= CONFIG =================
// WiFi credentials
const char *ssid = "AndroidAP66E5";
const char *password = "ohfa7243";
// MQTT broker details
const char *mqtt_broker = "54.79.62.237";
const int mqtt_port = 8883;
// Node ID identifies this device
// Example values:
// "left_arm", "right_arm", "left_leg", "right_leg"
const char *node_id = "right_arm";
// Player ID identifies which player the system belongs to (can be "shifu" or
// "student")
const char *player_id = "shifu";

// ================= MQTT TOPICS =================
// Topics for sending/receiving messages
String commandTopic = "command/" + String(player_id) + "/" +
                      String(node_id); // Commands sent from server
String sensorTopic = "sensor/" + String(player_id) + "/" + String(node_id) +
                     "/raw"; // Sensor data published by this node

// ================= GLOBAL OBJECTS =================
// Secure WiFi client for TLS
WiFiClientSecure espClient;
// MQTT client using secure connection
PubSubClient client(espClient);

#endif // COMMS_HEADER_H