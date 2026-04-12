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

// #define DEBUG
#define ENABLE_SENSOR_COMMS
#define ENABLE_WIFI_COMMS
#define ENABLE_HAPTICS

/**
 * Includes
 */
#include <Adafruit_AHRS.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_DRV2605.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

#include "definitions.h"

/**
 * Helper Structs
 */
typedef struct {
    double x, y, z; // World-frame linear acceleration (m/s^2, gravity removed)
    double roll, pitch,
        yaw; // World-frame angular velocity (rad/s)
} imu_reading_t;

typedef struct {
    double g_mag;         // Measured gravity magnitude (m/s^2)
    double gyro_bias[3];  // Gyro bias {bx, by, bz} in rad/s
} imu_calib_t;

/**
 * Global Variables
 */
// IMU Variables
const uint8_t MPU_DEVICE_IDS[NUM_IMU] = {MPU_UPPER_ADDR, MPU_LOWER_ADDR};

Adafruit_MPU6050 mpu_devices[NUM_IMU];
Adafruit_Mahony filters[NUM_IMU];

// Battery Variables
typedef struct {
    double voltage;
    int percentage;
} batt_reading_t;

bool blink_state_batt = false;

// Heartbeat Variables
bool blink_state_hb = false;
uint8_t led_pwm = 0;
int8_t led_pwm_dir = 1;

// Haptics Variables
Adafruit_DRV2605 drv;

/**
 * RTOS Prototypes
 */


QueueHandle_t xIMUQueue[NUM_IMU] = {nullptr};
TaskHandle_t IMUTaskHandle[NUM_IMU] = {nullptr};
TaskHandle_t CommsWatchdogTaskHandle = NULL;
TaskHandle_t CommsSensorsTaskHandle = NULL;

/**
 * Event Group is fixed 24 bits
 * Static Event Group is used so that it is precompiled
 *
 * BITS 0-1: IMU 0-1 initialization status (1 = initialized, 0 = not initialized)
 * BITS 2-3: IMU 0-1 calibration pending status (1 = pending calibration, 0 = not pending)
 * BIT 4: COMMS_FLAG_BIT (1 = WiFi + MQTT connected)
 * BIT 5: COMMS_RUNNING_FLAG_BIT (1 = system actively sending data)
 * BIT 6: HAPTICS_ON_BIT (1 = haptic feedback triggered)
 */
EventGroupHandle_t xSystemEventGroup = NULL;
StaticEventGroup_t xSystemEventGroupBuffer;

QueueHandle_t xBattDispQueue = NULL;
QueueHandle_t xBattSerialQueue = NULL;

TaskHandle_t BatteryTaskHandle = NULL;
TaskHandle_t CommsBattTaskHandle = NULL;
TaskHandle_t BattDispTaskHandle = NULL;
TaskHandle_t hbDispTaskHandle = NULL;

SemaphoreHandle_t xSerialMutex = NULL;

TaskHandle_t hapticTaskHandle = NULL;

#if defined(DEBUG)
TaskHandle_t MonitorTaskHandle = NULL;
#endif

#endif // HEADER_H