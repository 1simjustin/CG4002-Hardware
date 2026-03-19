#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/**
 * Definitions
 */

// HW Definitions
#define COMMS_CORE 0
#define SENSOR_CORE 1

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// RTOS Definitions
#define STACK_SIZE 1024
#define IMU_STACK_SIZE 8192
#define COMMS_STACK_SIZE 16384

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

#define IMU_FREQ_HZ 50
#define IMU_PERIOD_MS (1000 / IMU_FREQ_HZ)

#define NUM_IMU 2
// Bits 0 to NUM_IMU-1 for IMU initialization status
#define IMU_FLAG_BITS ((1 << NUM_IMU) - 1)
// Bits NUM_IMU to NUM_IMU*2-1 for IMU calibration status
#define IMU_CALIB_FLAG_BITS IMU_FLAG_BITS << NUM_IMU
// Bit NUM_IMU*2 for overall system running status
#define COMMS_RUNNING_FLAG_BIT (1 << (NUM_IMU * 2))

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

#endif // DEFINITIONS_H