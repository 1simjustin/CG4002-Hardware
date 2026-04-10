#ifndef COMMS_HEADER_H
#define COMMS_HEADER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

// ================= CONFIG =================
// MQTT broker details
const char *mqtt_broker = "54.79.62.237";
const int mqtt_port = 8883;
// Node ID identifies this device
// Example values:
// "left_arm", "right_arm", "left_leg", "right_leg"
const char *node_id = NODE;
// Player ID identifies which player the system belongs to (can be "shifu" or
// "student")
#if (PLAYER == PLAYER_STUDENT)
const char *player_id = "student";
#elif (PLAYER == PLAYER_SHIFU)
const char *player_id = "shifu";
#endif

// Decide body part names automatically
// Set part 1 to "upper_arm" or "thigh" and part 2 to "forearm" or "shin" based on node_id
#define IS_ARM_NODE (strstr(NODE, "arm") != NULL)

char part1[32];
char part2[32];

// ================= MQTT TOPICS =================
// Topics for sending/receiving messages
char commandTopic[64];
char broadcastTopic[64];
char sensorTopic[64];
char statusTopic[64];
char inferenceTopic[64];

void buildTopics() {
    // Build body part names
    snprintf(part1, sizeof(part1), "%s%s", NODE, IS_ARM_NODE ? "_upper_arm" : "_thigh");
    snprintf(part2, sizeof(part2), "%s%s", NODE, IS_ARM_NODE ? "_forearm"   : "_shin");

    // Build MQTT topics
    snprintf(commandTopic,   sizeof(commandTopic),   "command/%s/%s",     player_id, node_id);
    snprintf(broadcastTopic, sizeof(broadcastTopic), "command/%s/all",    player_id);
    snprintf(sensorTopic,    sizeof(sensorTopic),    "sensor/%s/%s/raw",  player_id, node_id);
    snprintf(statusTopic,    sizeof(statusTopic),    "status/%s/%s",      player_id, node_id);
    snprintf(inferenceTopic, sizeof(inferenceTopic), "inference/%s/%s",   player_id, node_id);
}

// ================= NTP CONFIG =================
const char* ntp_server = "pool.ntp.org";

// ================= GLOBAL OBJECTS =================
// Secure WiFi client for TLS
WiFiClientSecure espClient;
// MQTT client using secure connection
PubSubClient client(espClient);

#endif // COMMS_HEADER_H