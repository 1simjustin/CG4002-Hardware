# ESP32 Wearable Motion Sensing System

CG4002 Computer Engineering Capstone Project — ESP32 FreeRTOS firmware for wearable IMU nodes.

## Overview

Each node is an ESP32 with dual MPU6050 IMUs (upper + lower limb), a battery monitor, heartbeat LED, battery LED, and optional haptic feedback (DRV2605). The system supports 8 nodes across 2 players (4 body parts each), communicating over MQTT/TLS to a central laptop server.

### System Architecture

```
Core 0 (Comms)                    Core 1 (Sensors)
+-----------------------+         +-----------------------+
| CommsWatchdogTask [3] |         | IMUTask0          [4] |
| CommsSensorsTask  [2] |         | IMUTask1          [4] |
| SerialBattTask    [1] |         | BatteryTask       [3] |
| BatteryDisplayTask[2] |         | HeartbeatDispTask [2] |
+-----------------------+         | HapticTask        [1] |
                                  +-----------------------+
Numbers in brackets = task priority
```

## File Structure

| File | Purpose |
|------|---------|
| `top.ino` | Project header and description |
| `definitions.h` | All configuration macros (pins, priorities, thresholds, per-node calibration) |
| `header.h` | Structs, build flags, global handles (queues, semaphores, event group) |
| `comms_header.h` | MQTT/WiFi config, TLS cert reference, topic builder |
| `0_certificate.ino` | TLS CA certificate and WiFi credentials |
| `0_prototypes.ino` | Forward declarations for all functions |
| `1_main.ino` | `setup()` — creates all FreeRTOS tasks; `loop()` is empty |
| `2_interrupts.ino` | Reserved for interrupt handlers (currently empty) |
| `3_setup.ino` | I2C init, semaphore/queue creation, event group init |
| `4a_ser_comms.ino` | Serial fallback: `serialSensorsTask`, `serialBattTask` |
| `4b_comms.ino` | WiFi/MQTT: watchdog, sensor publishing, MQTT callback, NTP sync |
| `5_imu.ino` | IMU setup, offset-based calibration, `imuTask` (50 Hz read loop) |
| `6_batt.ino` | Battery ADC reading, SOC calculation, display task |
| `7_haptics.ino` | DRV2605 setup, haptic pulse task |
| `8_heartbeat.ino` | Multi-state heartbeat LED (init/calib/wifi/running/idle) |
| `9_general.ino` | Debug monitor task (disabled by default) |

## Configuration

### Build Flags

Defined in `header.h`, toggled by commenting/uncommenting:

```c
// #define DEBUG              // Enable Serial debug prints
// #define USE_AHRS           // Use Mahony AHRS filter instead of raw gyro
#define ENABLE_SENSOR_COMMS   // Enable sensor data transmission
#define ENABLE_WIFI_COMMS     // Use WiFi/MQTT (else serial fallback)
#define ENABLE_HAPTICS        // Enable haptic feedback (student nodes only)
```

### Node Selection

In `definitions.h`, change these two values to configure which node this firmware targets:

```c
#define PLAYER PLAYER_SHIFU         // PLAYER_STUDENT or PLAYER_SHIFU
#define NODE_ID NODE_RIGHT_ARM      // NODE_LEFT_ARM, NODE_RIGHT_ARM, NODE_LEFT_LEG, NODE_RIGHT_LEG
```

`NODE` string and `VOLTAGE_DIV_RATIO` are derived automatically from `PLAYER` and `NODE_ID`.

### Hardware Pins

| Pin | Function |
|-----|----------|
| GPIO 21 (SDA) | I2C data (MPU6050 x2, DRV2605) |
| GPIO 22 (SCL) | I2C clock |
| A0 | Battery voltage ADC input |
| D2 | Battery status LED |
| D3 | Heartbeat status LED |
| D7 | Haptic motor PWM |

### IMU Addresses

| IMU | I2C Address | Placement |
|-----|-------------|-----------|
| Upper | `0x68` | Upper limb segment (upper arm / thigh) |
| Lower | `0x69` | Lower limb segment (forearm / shin) |

**Note:** The Adafruit MPU6050 library's `begin()` method performs a WHO_AM_I register check that expects the default MPU6050 device ID. Some compatible IMU chips (e.g. MPU6500, clone variants) return a different WHO_AM_I value, causing `begin()` to fail even though the sensor is fully functional. To use these chips, the WHO_AM_I check in the library must be removed or relaxed. Modify `Adafruit_MPU6050::begin()` in the installed library to skip or bypass the device ID validation.

## Event Group Bits

The system uses a 24-bit FreeRTOS event group for inter-task state signaling:

| Bit(s) | Name | Meaning |
|--------|------|---------|
| 0-1 | `IMU_FLAG_BITS` | IMU 0/1 initialized (1 = ready) |
| 2-3 | `IMU_CALIB_FLAG_BITS` | IMU 0/1 calibration pending (1 = needs calibration) |
| 4 | `COMMS_FLAG_BIT` | WiFi + MQTT connected |
| 5 | `COMMS_RUNNING_FLAG_BIT` | System actively sending data (after start command) |
| 6 | `HAPTICS_ON_BIT` | Haptic feedback triggered by low inference score |

## Queues and Semaphores

| Handle | Type | Item | Purpose |
|--------|------|------|---------|
| `xIMUQueue[0..1]` | Queue (size 1) | `imu_reading_t` | Latest IMU reading per sensor |
| `xBattDispQueue` | Queue (size 1) | `batt_reading_t` | Battery data for display task |
| `xBattSerialQueue` | Queue (size 1) | `batt_reading_t` | Battery data for serial output |
| `xSerialMutex` | Mutex | — | Protects `Serial.print()` across tasks |

All data queues use `xQueueOverwrite` (latest-value semantics, never blocks the producer).

## IMU Calibration

### Offset-Based Zeroing

At calibration time (device at rest), the firmware averages raw accelerometer and gyroscope readings to compute per-axis offsets. These offsets are subtracted from all subsequent readings.

**Process:**

1. Collect `CALIBRATION_SAMPLES` (10) readings at 50 Hz
2. Average accelerometer readings `(avg_ax, avg_ay, avg_az)` and gyroscope readings `(avg_gx, avg_gy, avg_gz)`
3. Store as `imu_reading_t` offsets

**Per-reading application (`applyCalibration`):**

```
a_calibrated = a_raw - a_offset
g_calibrated = g_raw - g_offset
```

### When Calibration Runs

- On IMU initialization (power-on)
- On receiving a `start` command from the server (re-calibration via `IMU_CALIB_FLAG_BITS`)

### Limitations

This approach assumes the device orientation at calibration time is fixed. Since gravity projects onto all 3 axes when the IMU is tilted, the subtracted offsets include the gravity component at the calibration orientation. If the device rotates during use, the residual gravity in the tilted axes introduces incorrect offsets.

## Communications

### MQTT Topics

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `command/{player}/{node}` | Server -> Node | Start/stop commands |
| `command/{player}/all` | Server -> All | Broadcast commands |
| `sensor/{player}/{node}/raw` | Node -> Server | IMU sensor data (JSON) |
| `status/{player}/{node}` | Node -> Server | Connection status |
| `inference/{player}/{node}` | Server -> Node | Inference scores for haptic feedback |

### Sensor Packet Format

```json
{
  "sequence": 1712937600000,
  "right_arm_upper_arm": {
    "accel": [0.12, -0.34, 0.05],
    "gyro": [0.01, -0.02, 0.03]
  },
  "right_arm_forearm": {
    "accel": [0.45, 0.12, -0.08],
    "gyro": [-0.01, 0.04, 0.00]
  }
}
```

- `sequence`: Unix timestamp in milliseconds (from NTP if synced, else `millis()`)
- `accel`: Acceleration in m/s^2 (offset-zeroed, includes residual gravity)
- `gyro`: Angular velocity in rad/s (or degrees if AHRS enabled)

### Connection Architecture

```
CommsWatchdogTask              CommsSensorsTask
 |                              |
 |-- monitors WiFi.status()     |-- blocks on COMMS_FLAG_BIT
 |-- monitors client.connected  |-- calls client.loop()
 |-- clears/sets COMMS_FLAG_BIT |-- publishes sensor data
 |-- calls connectWiFi()        |-- handles scheduled starts
 |-- calls connectMQTT()        |
```

PubSubClient is not thread-safe. `client.loop()` and `client.publish()` are both in `CommsSensorsTask`. The watchdog only manages connectivity state and never touches the MQTT client's message loop.

### NTP Sync

NTP is attempted at WiFi connection time (max 3 retries, 10s timeout each). If NTP fails:
- Scheduled start commands (`start_at` timestamp) are ignored
- The node starts sending data immediately on receiving a start command
- Packet timestamps use `millis()` instead of Unix time

### WiFi Reconnection

On WiFi loss, `CommsWatchdogTask` detects it within `COMMS_TASK_DELAY_MS` (10ms), clears `COMMS_FLAG_BIT | COMMS_RUNNING_FLAG_BIT`, and reconnects. This immediately:
- Stops data publishing (COMMS_FLAG_BIT guard)
- Switches heartbeat LED to fast blink (4 Hz)
- Stops haptic feedback

## Battery Monitoring

- Reads battery voltage via ADC every 1 second
- Uses per-node voltage divider ratio for accurate conversion
- SOC: linear interpolation between 3.0V (0%) and 4.2V (100%)
- **LED indicators:**
  - Off: battery > 50%
  - Dimming PWM: 20-50% (brightness proportional to charge)
  - Blinking: < 20% critical

## Heartbeat LED States

The heartbeat LED on `HB_LED_PIN` (D3) indicates system state:

| State | Pattern | Meaning |
|-------|---------|---------|
| IMU init pending | 1 Hz blink | Waiting for MPU6050 to initialize |
| Calibration pending | 2 Hz blink | IMU initialized, calibration in progress |
| WiFi/MQTT pending | 4 Hz blink | Calibrated, waiting for connection |
| Idle (connected) | Breathing | Connected but not running |
| Running | Solid on | Actively sending sensor data |

## Haptic Feedback

Available only on student nodes (`PLAYER_STUDENT` + `ENABLE_HAPTICS`).

- Triggered when inference confidence score < `HAPTIC_SCORE_THRESHOLD` (0.2) for either body segment
- Cleared when both scores >= threshold
- Requires both `HAPTICS_ON_BIT` and `COMMS_RUNNING_FLAG_BIT` to be active
- Pulse pattern: 200ms on (78% PWM), 800ms off (50% PWM), repeating
- Uses DRV2605 haptic driver in analog PWM mode

## Data Structures

```c
// Sensor reading (used in queues, sliding window, and calibration offsets)
typedef struct {
    double x, y, z;           // Accelerometer (m/s^2)
    double roll, pitch, yaw;  // Gyroscope (rad/s) or AHRS angles (deg)
} imu_reading_t;

// Battery reading
typedef struct {
    double voltage;   // Measured voltage (V)
    int percentage;   // State of charge (0-100%)
} batt_reading_t;
```
