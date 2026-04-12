/**
 * NUS AY25/26 Semester 2 CG4002 Computer Engineering Capstone Project
 * Author: Sim Justin (B12)
 * 
 * This file is the main Arduino sketch for the top section of the project.
 * The program is compiled and uploaded using the Arduino IDE.
 * 
 * Key Components:
 * - 2x MPU6050 IMU: Used for motion sensing and orientation detection.
 * - Analog Battery Voltage Reading: Monitors the battery voltage level.
 * - Battery Display: Visual representation of battery status.
 *   - Read from the Comms server and display only the lowest battery level.
 * - Heartbeat
 * - Wireless Communication: Sends sensor data to the Comms server
 * - Haptics: Sensory feedback after inferencing
 * 
 * Implementation consists of ESP-IDF FreeRTOS tasks for parallel and
 * concurrent operations, utilising 2 cores of the ESP32.
 * We reserve Core 0 for sensor reading and data processing,
 * while Core 1 is used for wireless communication and battery display.
 */