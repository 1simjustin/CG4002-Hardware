#include "definitions.h"
#include "header.h"

/**
 * Upper Limb MPU Addr - 0x68
 * Lower Limb MPU Addr - 0x69
 */

double radToDeg(double rad) {
    double deg = rad * RAD_TO_DEG;
    return deg;
}

double degToRad(double deg) {
    double rad = deg / RAD_TO_DEG;
    return rad;
}

// Return array of 3 sensor_event_t for accel, gyro, temp respectively
imu_reading_t calibrate(Adafruit_MPU6050 &mpu) {
    sensors_event_t a_samples[CALIBRATION_SAMPLES];
    sensors_event_t g_samples[CALIBRATION_SAMPLES];
    sensors_event_t t_samples[CALIBRATION_SAMPLES];

    // Collect samples for calibration
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        mpu.getEvent(&a_samples[i], &g_samples[i], &t_samples[i]);
        delay(CALIBRATION_DELAY_MS);
    }

    // Compute average for each sensor
    imu_reading_t offsets = {0};
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        offsets.x += a_samples[i].acceleration.x;
        offsets.y += a_samples[i].acceleration.y;
        offsets.z += a_samples[i].acceleration.z;

        offsets.roll += g_samples[i].gyro.x;
        offsets.pitch += g_samples[i].gyro.y;
        offsets.yaw += g_samples[i].gyro.z;
    }
    offsets.x /= CALIBRATION_SAMPLES;
    offsets.y /= CALIBRATION_SAMPLES;
    offsets.z /= CALIBRATION_SAMPLES;
    offsets.roll /= CALIBRATION_SAMPLES;
    offsets.pitch /= CALIBRATION_SAMPLES;
    offsets.yaw /= CALIBRATION_SAMPLES;

    return offsets;
}

void applyCalibration(sensors_event_t *a, sensors_event_t *g,
                      imu_reading_t *calib) {
    a->acceleration.x -= calib->x;
    a->acceleration.y -= calib->y;
    a->acceleration.z -= calib->z;

    g->gyro.x -= calib->roll;
    g->gyro.y -= calib->pitch;
    g->gyro.z -= calib->yaw;
}

imu_reading_t windowAvg(imu_reading_t *readings, int size) {
    double sum_ax = 0, sum_ay = 0, sum_az = 0;
    double sum_gx = 0, sum_gy = 0, sum_gz = 0;
    imu_reading_t result = {0};

    for (int i = 0; i < size; i++) {
        sum_ax += readings[i].x;
        sum_ay += readings[i].y;
        sum_az += readings[i].z;
        sum_gx += readings[i].roll;
        sum_gy += readings[i].pitch;
        sum_gz += readings[i].yaw;
    }

    result.x = sum_ax / size;
    result.y = sum_ay / size;
    result.z = sum_az / size;
    result.roll = sum_gx / size;
    result.pitch = sum_gy / size;
    result.yaw = sum_gz / size;

    return result;
}

bool imu_setup(int idx) {
    if (!mpu_devices[idx].begin(MPU_DEVICE_IDS[idx])) {
        Serial.print("Failed to find MPU6050 chip for IMU ");
        Serial.println(idx);
        return false;
    }

#if defined(DEBUG)
    Serial.print("MPU6050 Found for IMU ");
    Serial.println(idx);
#endif

    mpu_devices[idx].setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu_devices[idx].setGyroRange(MPU6050_RANGE_500_DEG);
    mpu_devices[idx].setFilterBandwidth(MPU6050_BAND_21_HZ);

    return true;
}

void filter_setup(int idx) {
    filters[idx].begin(IMU_FREQ_HZ);
    return;
}

/**
 * TASK DEFINITIONS
 */

void imuTask(void *parameter) {
    int imu_id = (int)parameter;
    bool imu_success = imu_setup(imu_id);
    if (!imu_success) {
        imu_init[imu_id] = false;
        vTaskDelete(NULL); // Delete task if IMU setup fails
    }

// Setup IMU + Filter
#if defined(USE_AHRS)
    filter_setup(imu_id);
#endif
    imu_reading_t calib_offsets = calibrate(mpu_devices[imu_id]);
    // imu_reading_t calib_offsets = {0};
    sensors_event_t a, g, temp;
    imu_reading_t imu_buffer[SLIDING_WINDOW_SIZE] = {0};

    int window_idx = 0;
    int sz = 0;
    imu_init[imu_id] = true;

#if defined(DEBUG)
    Serial.print("Calibration complete for IMU ");
    Serial.println(imu_id);
#endif

    for (;;) {
        mpu_devices[imu_id].getEvent(&a, &g, &temp);
        applyCalibration(&a, &g, &calib_offsets);

        // Store raw readings in buffer for sliding window average
        imu_buffer[window_idx].x = a.acceleration.x;
        imu_buffer[window_idx].y = a.acceleration.y;
        imu_buffer[window_idx].z = a.acceleration.z;

#ifdef USE_AHRS
        filters[imu_id].update(g.gyro.x, g.gyro.y, g.gyro.z, a.acceleration.x,
                               a.acceleration.y, a.acceleration.z);
        imu_buffer[window_idx].roll = filters[imu_id].getRoll();
        imu_buffer[window_idx].pitch = filters[imu_id].getPitch();
        imu_buffer[window_idx].yaw = filters[imu_id].getYaw();
#else
        imu_buffer[window_idx].roll = g.gyro.x;
        imu_buffer[window_idx].pitch = g.gyro.y;
        imu_buffer[window_idx].yaw = g.gyro.z;
#endif

        // Update indices
        sz = min(sz + 1, SLIDING_WINDOW_SIZE);
        window_idx = (window_idx + 1) % SLIDING_WINDOW_SIZE;

        // Compute sliding window average and send to queue
        imu_reading_t sensor_result = windowAvg(imu_buffer, sz);
        xQueueOverwrite(xIMUQueue[imu_id], &sensor_result);
        vTaskDelay(pdMS_TO_TICKS(1000 / IMU_FREQ_HZ));
    }
}