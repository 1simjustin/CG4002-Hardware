#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/**
 * Definitions
 */

// User Configurations
#define PLAYER_STUDENT 1
#define PLAYER_SHIFU 2
// Toggle the below line to switch between "student" and "shifu" player
#define PLAYER PLAYER_SHIFU

// Node identifiers (numeric for preprocessor comparison)
#define NODE_LEFT_ARM  1
#define NODE_RIGHT_ARM 2
#define NODE_LEFT_LEG  3
#define NODE_RIGHT_LEG 4
// Toggle the below line to switch between nodes
#define NODE_ID NODE_LEFT_LEG

// Derive NODE string from NODE_ID
#if (NODE_ID == NODE_LEFT_ARM)
    #define NODE "left_arm"
#elif (NODE_ID == NODE_RIGHT_ARM)
    #define NODE "right_arm"
#elif (NODE_ID == NODE_LEFT_LEG)
    #define NODE "left_leg"
#elif (NODE_ID == NODE_RIGHT_LEG)
    #define NODE "right_leg"
#endif

// HW Definitions
#define COMMS_CORE 0
#define SENSOR_CORE 1

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// RTOS Definitions
#define STACK_SIZE 8192
#define COMMS_STACK_SIZE 16384

// Core 1 (Sensors) Task Priorities
#define IMU_TASK_PRIORITY 4
#define BATT_TASK_PRIORITY 3
#define ACTUATOR_TASK_PRIORITY 2
#define HAPTIC_TASK_PRIORITY 1

// Core 0 (Comms) Task Priorities
#define COMMS_WDG_TASK_PRIORITY 3
#define SENSORS_COMMS_TASK_PRIORITY 2
#define BATT_COMMS_TASK_PRIORITY 1

#define COMMS_TASK_DELAY_MS 10
#define NTP_TIMEOUT_MS 10000
#define NTP_MAX_RETRIES 3
#define WIFI_RETRY_ATTEMPTS 10

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
#define IMU_CALIB_FLAG_BITS (IMU_FLAG_BITS << NUM_IMU)
// Bit NUM_IMU*2 for wifi and mqtt connection status
#define COMMS_FLAG_BIT (1 << (NUM_IMU * 2))
// Bit NUM_IMU*2+1 for overall system running status
#define COMMS_RUNNING_FLAG_BIT (1 << (NUM_IMU * 2 + 1))
// Bit NUM_IMU*2+2 for haptics on status
#define HAPTICS_ON_BIT (1 << (NUM_IMU * 2 + 2))
// Bit NUM_IMU*2+3 for NTP sync status (1 = NTP synced, 0 = no NTP)
#define NTP_SYNCED_BIT (1 << (NUM_IMU * 2 + 3))

#define CALIBRATION_SAMPLES 10
#define CALIBRATION_DELAY_MS IMU_PERIOD_MS
#define GRAVITY_ACCEL 9.80665 // Standard gravity m/s^2
#define MAHONY_KP 5.0f  // Proportional gain (default 0.5, higher = faster convergence)
#define MAHONY_KI 0.1f  // Integral gain (default 0.0, small value corrects gyro drift)
#define SLIDING_WINDOW_SIZE 1

// Batt Reader Definitions
#define BATTERY_PIN A0
#define ADC_MAX 4095 // 12-bit ADC
// Per-node voltage divider ratios — measured (R1+R2)/R2
#if (PLAYER == PLAYER_SHIFU)
    #if (NODE_ID == NODE_LEFT_ARM)
        #define VOLTAGE_DIV_RATIO 2.0556
    #elif (NODE_ID == NODE_RIGHT_ARM)
        #define VOLTAGE_DIV_RATIO 2.0553
    #elif (NODE_ID == NODE_LEFT_LEG)
        #define VOLTAGE_DIV_RATIO 2.0594
    #elif (NODE_ID == NODE_RIGHT_LEG)
        #define VOLTAGE_DIV_RATIO 2.0599
    #endif
#elif (PLAYER == PLAYER_STUDENT)
    #if (NODE_ID == NODE_LEFT_ARM)
        #define VOLTAGE_DIV_RATIO 2.3 // 2.1010 // Original is commented to make it work
    #elif (NODE_ID == NODE_RIGHT_ARM)
        #define VOLTAGE_DIV_RATIO 2.0467
    #elif (NODE_ID == NODE_LEFT_LEG)
        #define VOLTAGE_DIV_RATIO 2.0648
    #elif (NODE_ID == NODE_RIGHT_LEG)
        #define VOLTAGE_DIV_RATIO 2.0596
    #endif
#endif
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
#define HB_INIT_BLINK_PERIOD_MS 1000
#define HB_CALIB_BLINK_PERIOD_MS 500
#define HB_WIFI_BLINK_PERIOD_MS 250
#define HB_RUNNING_NO_NTP_PWM 64

// Haptics Definitions
#define HAPTIC_PIN D7
#define HAPTIC_PULSE_MS 200
#define HAPTIC_PAUSE_MS 800
#define HAPTIC_OFF_PWM 127
#define HAPTIC_ON_PWM 200
#define HAPTIC_POLL_TIMER 200
#define HAPTIC_SCORE_THRESHOLD 0.2
#define HAPTIC_TRIG_BITS (HAPTICS_ON_BIT | COMMS_RUNNING_FLAG_BIT)

#endif // DEFINITIONS_H