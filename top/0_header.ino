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
// #define USE_AHRS

// #define TORSO_DEVICE
// #define LEG

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

#define IMU_TASK_PRIORITY 4
#define FLEX_TASK_PRIORITY 3
#define BATT_TASK_PRIORITY 2
#define ACTUATOR_TASK_PRIORITY 1

#define SENSORS_COMMS_TASK_PRIORITY 5
#define BATT_COMMS_TASK_PRIORITY 4

// IMU Definitions
#define MPU6050_DEVICE_ID 0x68
#define RAD_TO_DEG 57.2957795131

#define IMU_FREQ_HZ 150
#define IMU_PERIOD_MS (1000 / IMU_FREQ_HZ)

#define NUM_IMUS 0
#ifdef TORSO_DEVICE
#define NUM_IMUS 3
#define CHEST_IMU_ID (NUM_IMUS - 1) // Last channel on Mux
#define I2C_ADDR_SEL_PIN1 25
#define I2C_ADDR_SEL_PIN2 26
#endif
#ifdef LEG_DEVICE
#define NUM_IMUS 1
#endif

// Flex Sensor Definitions
#define FLEX_FREQ_HZ 150
#define FLEX_PERIOD_MS (1000 / FLEX_FREQ_HZ)

#define NUM_FLEX 0
#ifdef TORSO_DEVICE
#define NUM_FLEX 2
#define FLEX_PIN1 A1
#define FLEX_PIN2 A2
#endif
#ifdef LEG_DEVICE
#define NUM_FLEX 1
#define FLEX_PIN A1
#endif

// Batt Reader Definitions
#define BATTERY_PIN A0
#define ADC_MAX 4095 // 12-bit ADC
#define VOLTAGE_DIVIDER_RATIO 2.0
#define ADC_REF_VOLTAGE 3.3
#define BATT_PERIOD_MS 1000

/**
 * Global Variables
 */

// IMU Variables
Adafruit_MPU6050 mpu_devices[NUM_IMUS];
Adafruit_Mahony filters[NUM_IMUS];
sensors_event_t a_arr[NUM_IMUS], g_arr[NUM_IMUS];
double x_out[NUM_IMUS], y_out[NUM_IMUS], z_out[NUM_IMUS];
double roll_out[NUM_IMUS], pitch_out[NUM_IMUS], yaw_out[NUM_IMUS];

// Flex Sensor Variables
int flex_readings[NUM_FLEX];
double flex_output[NUM_FLEX];

#ifdef TORSO_DEVICE
const int FLEX_PINS[NUM_FLEX] = {FLEX_PIN1, FLEX_PIN2};
#endif
#ifdef LEG_DEVICE
const int FLEX_PINS[NUM_FLEX] = {FLEX_PIN};
#endif

// Battery Variables
double batt_voltage;

/**
 * RTOS Prototypes
 */

#if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
SemaphoreHandle_t xIMUSemaphore[NUM_IMUS] = { nullptr };
SemaphoreHandle_t xFlexSemaphore[NUM_FLEX] = { nullptr };

TaskHandle_t IMUTaskHandle[NUM_IMUS] = { nullptr };
TaskHandle_t FlexTaskHandle[NUM_FLEX] = { nullptr };
TaskHandle_t CommsSensorsTaskHandle[NUM_IMUS] = { nullptr };
#endif

SemaphoreHandle_t xBattSemaphore = NULL;

TaskHandle_t BatteryTaskHandle = NULL;
TaskHandle_t CommsBattTaskHandle = NULL;

/**
 * Generic Function Prototypes
 */

// Setup Function Prototypes
void i2c_setup();
void imu_setup();
void flex_setup();
void voltage_reader_setup();
void createSemaphores();

// Comms Function Prototypes
void serialSensorsTask(void *parameter);
void serialBattTask(void *parameter);
void commsSensorsTask(void *parameter);
void commsBattTask(void *parameter);

// Sensors Function Prototypes
void imuTask(void *parameter);
double processFlexReading(int raw_reading);
void flexTask(void *parameter);
void battTask(void *parameter);

// Actuators Function Prototypes

// Generic Function Prototypes
double radToDeg(double rad);
#ifdef TORSO_DEVICE
void set_mux_channel(int channel);
#endif