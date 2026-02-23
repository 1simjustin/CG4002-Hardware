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

// #define TORSO_DEVICE
#define LEG_DEVICE

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

// Hardware Definitions
#define COMMS_CORE 0
#define SENSOR_CORE 1

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// RTOS Definitions
#define STACK_SIZE 10000
#define IMU_TASK_PRIORITY 3
#define FLEX_TASK_PRIORITY 2
#define BATT_TASK_PRIORITY 1

// IMU Definitions
// Multiple IMU is only used for TORSO_DEVICE, but we define both for simplicity
#define MPU6050_DEVICE_ID1 0x70
#define MPU6050_DEVICE_ID2 0x68
#define RAD_TO_DEG 57.2957795131

#define IMU_FREQ_HZ 150
#define IMU_PERIOD_MS (1000 / IMU_FREQ_HZ)

#ifdef TORSO_DEVICE
#define NUM_IMUS 3
#define I2C_ADDR_SEL_PIN1 25
#define I2C_ADDR_SEL_PIN2 26
#endif

// Flex Sensor Definitions
#define FLEX_FREQ_HZ 150
#define FLEX_PERIOD_MS (1000 / FLEX_FREQ_HZ)

#ifdef TORSO_DEVICE
#define NUM_FLEX 2
#define FLEX_PIN1 A1
#define FLEX_PIN2 A2
#endif

#ifdef LEG_DEVICE
#define NUM_FLEX 1
#define FLEX_PIN A1
#endif

// Battery Voltage Reading
// Battery voltage is read as an analog value and converted to actual voltage using a voltage divider
// We use a 1:1 voltage divider for simplicity
#define BATTERY_PIN A0
#define ADC_MAX 4095 // 12-bit ADC
#define VOLTAGE_DIVIDER_RATIO 2.0
#define ADC_REF_VOLTAGE 3.3
#define BATT_PERIOD_MS 1000

/**
 * Global Variables
 */

// IMU Vars
#ifdef TORSO_DEVICE
Adafruit_MPU6050 mpu[NUM_IMUS];
Adafruit_Mahony filter[NUM_IMUS];
sensors_event_t a_arr[NUM_IMUS], g_arr[NUM_IMUS];
// Switch betweem 0 and 1 for IMU on each arm
uint8_t mpu_id = 0;
#endif
#ifdef LEG_DEVICE
Adafruit_MPU6050 mpu;
Adafruit_Mahony filter;
sensors_event_t a, g;
#endif

// Flex Vars
int flex_readings[NUM_FLEX];
int flex_output[NUM_FLEX];

#ifdef TORSO_DEVICE
const int FLEX_PINS[NUM_FLEX] = {FLEX_PIN1, FLEX_PIN2};
#endif
#ifdef LEG_DEVICE
const int FLEX_PINS[NUM_FLEX] = {FLEX_PIN};
#endif

// Battery Voltage Vars
double batt_voltage;

/**
 * RTOS Task Prototypes
 */

#ifdef TORSO_DEVICE
#endif

#ifdef LEG_DEVICE
TaskHandle_t IMUTaskHandle = NULL;
TaskHandle_t FlexTaskHandle = NULL;
TaskHandle_t BatteryTaskHandle = NULL;
#endif

/**
 * Generic Function Prototypes
 */

// Setup Function Prototypes
void i2c_setup();
void imu_setup();
void flex_setup();
void voltage_reader_setup();

// Comms Function Prototypes
void commsTask(void *parameter);

// Sensors Function Prototypes
double radToDeg(double rad);
void imuTask(void *parameter);
void flexTask(void *parameter);
void battTask(void *parameter);

// Actuators Function Prototypes