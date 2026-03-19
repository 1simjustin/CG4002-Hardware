/**
 * This file is part of NUS CG4002 Computer Engineering Capstone Project
 * Author: Sim Justin (B12)
 *
 * This file contains the header information and global definitions
 * for the top section Arduino sketch.
 */

#ifndef HEADER_H
#define HEADER_H

/**
 * Pseudo Build Flags
 * We use this section to define pseudo build flags that can be
 * toggled for different build configurations via comments.
 */

#define DEBUG
// #define USE_AHRS
#define ENABLE_SENSOR_COMMS
#define ENABLE_WIFI_COMMS

/**
 * Includes
 */
#include <Adafruit_AHRS.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "definitions.h"

/**
 * Helper Structs
 */
typedef struct {
    double x, y, z; // Accelerometer readings
    double roll, pitch,
        yaw; // Gyro readings (or AHRS angles if USE_AHRS is defined)
} imu_reading_t;

/**
 * Global Variables
 */
// IMU Variables
const uint8_t MPU_DEVICE_IDS[NUM_IMU] = {MPU_UPPER_ADDR, MPU_LOWER_ADDR};

Adafruit_MPU6050 mpu_devices[NUM_IMU];
Adafruit_Mahony filters[NUM_IMU];

// Battery Variables
double batt_voltage;
int batt_percentage;
bool blink_state_batt = false;

// Heartbeat Variables
bool hb_blink = true;
bool blink_state_hb = false;

/**
 * RTOS Prototypes
 */

BaseType_t result;

QueueHandle_t xIMUQueue[NUM_IMU] = {nullptr};
TaskHandle_t IMUTaskHandle[NUM_IMU] = {nullptr};
TaskHandle_t CommsSensorsTaskHandle = NULL;

// Event Group is fixed 24 bits
// Event Group is used as static so that it is precompiled
// Bit 0 to NUM_IMU-1 indicate if IMU is initialized
// Bit NUM_IMU to NUM_IMU*2-1 indicate if IMU is pending calibration
// Bit NUM_IMU*2 indicates if system is running (after scheduled start time)
EventGroupHandle_t xIMUEventGroup = NULL;
StaticEventGroup_t xIMUEventGroupBuffer;

SemaphoreHandle_t xBattSemaphore = NULL;

TaskHandle_t BatteryTaskHandle = NULL;
TaskHandle_t CommsBattTaskHandle = NULL;
TaskHandle_t BattDispTaskHandle = NULL;
TaskHandle_t hbDispTaskHandle = NULL;

SemaphoreHandle_t xSerialMutex = NULL;

#if defined(DEBUG)
TaskHandle_t MonitorTaskHandle = NULL;
#endif

#endif // HEADER_H