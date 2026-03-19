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

// Decide body part names automatically
// Set part 1 to "upper_arm" or "thigh" and part 2 to "forearm" or "shin" based on node_id
const char* part1 = (String(node_id).indexOf("arm") >= 0) ? "_upper_arm" : "_thigh";
const char* part2 = (String(node_id).indexOf("arm") >= 0) ? "_forearm"   : "_shin";

// ================= MQTT TOPICS =================
// Topics for sending/receiving messages
String commandTopic   = "command/" + String(player_id) + "/" + String(node_id); // Node-specific commands
String broadcastTopic = "command/" + String(player_id) + "/all";                // Broadcast to all nodes
String sensorTopic    = "sensor/"  + String(player_id) + "/" + String(node_id) + "/raw";
String statusTopic    = "status/"  + String(player_id) + "/" + String(node_id); // Reconnect notifications

// ================= NTP CONFIG =================
const char* ntp_server = "pool.ntp.org";

// ================= GLOBAL OBJECTS =================
// Secure WiFi client for TLS
WiFiClientSecure espClient;
// MQTT client using secure connection
PubSubClient client(espClient);

#endif // COMMS_HEADER_H