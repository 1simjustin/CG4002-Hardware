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
#define ENABLE_SENSOR_COMMS

// #define DEVICE_ID // For identification in Comms

/**
 * Includes
 */
#include <Adafruit_AHRS.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/**
 * Definitions
 */

// HW Definitions
#define COMMS_CORE 0
#define SENSOR_CORE 1

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// RTOS Definitions
#define STACK_SIZE 4096

// Core 1 (Sensors) Task Priorities
#define IMU_TASK_PRIORITY 3
#define BATT_TASK_PRIORITY 2
#define ACTUATOR_TASK_PRIORITY 1

// Core 0 (Comms) Task Priorities
#define SENSORS_COMMS_TASK_PRIORITY 2
#define BATT_COMMS_TASK_PRIORITY 1

// IMU Definitions
#define MPU_UPPER_ADDR 0x68
#define MPU_LOWER_ADDR 0x69
#define RAD_TO_DEG 57.2957795131

#define IMU_FREQ_HZ 150
#define IMU_PERIOD_MS (1000 / IMU_FREQ_HZ)

#define NUM_IMU 2

#define CALIBRATION_SAMPLES 10
#define CALIBRATION_DELAY_MS 1
#define SLIDING_WINDOW_SIZE 1

// Batt Reader Definitions
#define BATTERY_PIN A0
#define ADC_MAX 4095 // 12-bit ADC
#define VOLTAGE_DIV_RATIO 2.0
#define ADC_REF 3.3
#define BATT_PERIOD_MS 1000
#define BATT_VOLT_MAX 4.2
#define BATT_VOLT_MIN 3.0

// Batt Display Definitions
#define BATT_LED_PIN D2
#define BATT_LOW_THRESHOLD 50
#define BATT_CRITICAL_THRESHOLD 20

// Heartbeat Display Definitions
#define HB_LED_PIN D3
#define HB_BLINK_PERIOD_MS 500

/**
 * Helper Structs
 */
typedef struct {
    sensors_event_t acceleration;
    sensors_event_t gyro;
} calib_data_t;

typedef struct {
    double avg_x, avg_y, avg_z;
    double avg_roll, avg_pitch, avg_yaw;
} data_out_t;

/**
 * Global Variables
 */
// IMU Variables
const uint8_t MPU_DEVICE_IDS[NUM_IMU] = {MPU_UPPER_ADDR, MPU_LOWER_ADDR};

Adafruit_MPU6050 mpu_devices[NUM_IMU];
Adafruit_Mahony filters[NUM_IMU];
sensors_event_t a_arr[NUM_IMU][SLIDING_WINDOW_SIZE];
sensors_event_t g_arr[NUM_IMU][SLIDING_WINDOW_SIZE];
#if defined(USE_AHRS)
    double roll_out[NUM_IMU][SLIDING_WINDOW_SIZE];
    double pitch_out[NUM_IMU][SLIDING_WINDOW_SIZE];
    double yaw_out[NUM_IMU][SLIDING_WINDOW_SIZE];
#endif
sensors_event_t temp[NUM_IMU][SLIDING_WINDOW_SIZE];
data_out_t data_out[NUM_IMU];
bool imu_init[NUM_IMU] = {false};

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

SemaphoreHandle_t xIMUSemaphore[NUM_IMU] = {nullptr};
TaskHandle_t IMUTaskHandle[NUM_IMU] = {nullptr};
TaskHandle_t CommsSensorsTaskHandle = NULL;

SemaphoreHandle_t xBattSemaphore = NULL;

TaskHandle_t BatteryTaskHandle = NULL;
TaskHandle_t CommsBattTaskHandle = NULL;
TaskHandle_t BattDispTaskHandle = NULL;
TaskHandle_t hbDispTaskHandle = NULL;

SemaphoreHandle_t xSerialMutex = NULL;

#if defined(DEBUG)
TaskHandle_t MonitorTaskHandle = NULL;
#endif

/**
 * Generic Function Prototypes
 */

// Setup Functions
void i2c_setup();
bool imu_setup(int idx);
void filter_setup(int idx);
void voltage_reader_setup();
void createSemaphores();

// Comms Functions
void serialSensorsTask(void *parameter);
void serialBattTask(void *parameter);
void commsSensorsTask(void *parameter);
void commsBattTask(void *parameter);

// Sensor Functions
void imuTask(void *parameter);
void calibrate(int idx, calib_data_t *calib_data);
void applyCalibration(sensors_event_t *a, sensors_event_t *g, calib_data_t *calib);
void windowAvg(int idx, int sz);

// Battery Functions
int batt_soc(double voltage);
void battTask(void *parameter);
void battDispTask(void *parameter);

// Heartbeat Functions
void hbDispTask(void *parameter);

// General Utility Functions
double radToDeg(double rad);
double degToRad(double deg);

#if defined(DEBUG)
void monitorTask(void *parameter);
#endif