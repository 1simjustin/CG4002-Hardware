/**
 * This file is part of NUS CG4002 Computer Engineering Capstone Project
 * Author: Sim Justin (B12)
 * 
 * This file contains the header information and global definitions
 * for the top section Arduino sketch.
 */

/**
 * Pseudo Build Flags
 * We use this section to define pseudo build flags that can be
 * toggled for different build configurations via comments.
 */

// #define DEBUG
// #define USE_FUSION
// #define USE_AHRS

#define TORSO_DEVICE
// #define LEG_DEVICE

// #define DEVICE_ID // For identification in Comms

/**
 * Includes
 */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHRS.h>
#include <Wire.h>

/**
 * Definitions
 */

// IMU Definitions
// Multiple IMU is only used for TORSO_DEVICE, but we define both for simplicity
#define MPU6050_DEVICE_ID1 0x70
#define MPU6050_DEVICE_ID2 0x68
#define RAD_TO_DEG 57.2957795131

// Flex Sensor Definitions
#define FLEX_PIN1 A1
#define FLEX_PIN2 A2

// Battery Voltage Reading
#define BATTERY_PIN A0

/**
 * Global Variables
 */

// IMU Vars
Adafruit_MPU6050 mpu[3];
Adafruit_Mahony filter1;
Adafruit_Mahony filter2;
sensors_event_t a_arr[3], g_arr[3];

// Switch betweem 0 and 1 for IMU on each arm
uint8_t mpu_id = 0;

// Flex Vars
int flex1, flex2;

// Battery Voltage Vars
// Battery voltage is read as an analog value and converted to actual voltage using a voltage divider
// We use a 1:1 voltage divider for simplicity
float battery_voltage;

/**
 * RTOS Task Prototypes
 */

/**
 * Generic Function Prototypes
 */

// Setup Function Prototypes
void i2c_setup();

// Comms Function Prototypes

// Sensors Function Prototypes
double radToDeg(double rad);

// Actuators Function Prototypes