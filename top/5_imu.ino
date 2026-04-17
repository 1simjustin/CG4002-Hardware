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

// Convert quaternion (body→world) to 3x3 rotation matrix
void quaternionToRotationMatrix(float qw, float qx, float qy, float qz,
                                double R[3][3]) {
    R[0][0] = 1.0 - 2.0 * (qy * qy + qz * qz);
    R[0][1] = 2.0 * (qx * qy - qw * qz);
    R[0][2] = 2.0 * (qx * qz + qw * qy);

    R[1][0] = 2.0 * (qx * qy + qw * qz);
    R[1][1] = 1.0 - 2.0 * (qx * qx + qz * qz);
    R[1][2] = 2.0 * (qy * qz - qw * qx);

    R[2][0] = 2.0 * (qx * qz - qw * qy);
    R[2][1] = 2.0 * (qy * qz + qw * qx);
    R[2][2] = 1.0 - 2.0 * (qx * qx + qy * qy);
}

// Calibrate IMU: compute gyro biases and gravity magnitude
imu_calib_t calibrate(Adafruit_MPU6050 &mpu) {
    sensors_event_t a, g, temp;
    double sum_ax = 0, sum_ay = 0, sum_az = 0;
    double sum_gx = 0, sum_gy = 0, sum_gz = 0;

    // Collect samples and accumulate running sums
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        mpu.getEvent(&a, &g, &temp);
        sum_ax += a.acceleration.x;
        sum_ay += a.acceleration.y;
        sum_az += a.acceleration.z;
        sum_gx += g.gyro.x;
        sum_gy += g.gyro.y;
        sum_gz += g.gyro.z;
        delay(CALIBRATION_DELAY_MS);
    }

    // Compute averages
    double avg_ax = sum_ax / CALIBRATION_SAMPLES;
    double avg_ay = sum_ay / CALIBRATION_SAMPLES;
    double avg_az = sum_az / CALIBRATION_SAMPLES;

    imu_calib_t calib = {0};

    // Gyro biases (measured in sensor frame)
    calib.gyro_bias[0] = sum_gx / CALIBRATION_SAMPLES;
    calib.gyro_bias[1] = sum_gy / CALIBRATION_SAMPLES;
    calib.gyro_bias[2] = sum_gz / CALIBRATION_SAMPLES;

    // Gravity magnitude from measured vector
    calib.g_mag = sqrt(avg_ax * avg_ax + avg_ay * avg_ay + avg_az * avg_az);

    return calib;
}

void applyCalibration(sensors_event_t *a, sensors_event_t *g,
                      imu_calib_t *calib) {
    // Only subtract gyro bias in sensor frame
    // World-frame rotation is applied after the Mahony filter update
    g->gyro.x -= calib->gyro_bias[0];
    g->gyro.y -= calib->gyro_bias[1];
    g->gyro.z -= calib->gyro_bias[2];
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
#if defined(DEBUG)
        Serial.print("Failed to find MPU6050 chip for IMU ");
        Serial.println(idx);
#endif
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
    filters[idx].setKp(MAHONY_KP);
    filters[idx].setKi(MAHONY_KI);
}

/**
 * TASK DEFINITIONS
 */

void imuTask(void *parameter) {
    int imu_id = (int)parameter;

    bool imu_success;
    EventBits_t imu_flag_bits;
    const EventBits_t calibration_bit = (1 << (imu_id + NUM_IMU));

    sensors_event_t a = {0}, g = {0}, temp = {0};
    imu_calib_t calib_offsets = {0};
    imu_reading_t imu_buffer[SLIDING_WINDOW_SIZE] = {0};

    int window_idx = 0;
    int sz = 0;

    for (;;) {
        // Try to initialise IMU before starting main loop.
        // if imu bit is 0, try to initialise
        if ((xEventGroupGetBits(xSystemEventGroup) & (1 << imu_id)) == 0) {
            imu_success = imu_setup(imu_id);
            // Perform initialisation
            if (imu_success) {
                filter_setup(imu_id);
                // Set IMU bit to 1 to indicate IMU is initialized
                xEventGroupSetBits(xSystemEventGroup, (1 << imu_id));
                // Set calibration bit to 1 to indicate IMU is initialized but
                // pending calibration
                xEventGroupSetBits(xSystemEventGroup, calibration_bit);
            } else {
                // Wait before retrying
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
        }

        // Check if IMU is pending calibration (calibration bit is 1)
        imu_flag_bits = xEventGroupWaitBits(
            xSystemEventGroup, // Event group handle
            calibration_bit,   // Bits to wait for (IMU calibration bit)
            pdTRUE,            // Clear bits on exit
            pdFALSE,           // Wait for any bit (not all)
            0                  // Non-blocking
        );
        // If calibration bit was set, perform calibration
        if (imu_flag_bits & calibration_bit) {
            calib_offsets = calibrate(mpu_devices[imu_id]);
#if defined(DEBUG)
            Serial.print("Calibration complete for IMU ");
            Serial.println(imu_id);
#endif
        }

        // Check if system is running (running bit is 1) before reading sensors
        if ((xEventGroupGetBits(xSystemEventGroup) & COMMS_RUNNING_FLAG_BIT) ==
            0) {
            // If not running, wait longer before checking again (power saving)
            vTaskDelay(pdMS_TO_TICKS(IDLE_IMU_POLL_MS));
            continue;
        }

        if (!mpu_devices[imu_id].getEvent(&a, &g, &temp)) {
            vTaskDelay(pdMS_TO_TICKS(IMU_PERIOD_MS));
            continue;
        }
        applyCalibration(&a, &g, &calib_offsets);

        // Feed bias-corrected gyro + raw accel (with gravity) into Mahony filter
        filters[imu_id].updateIMU(g.gyro.x, g.gyro.y, g.gyro.z,
                                  a.acceleration.x, a.acceleration.y,
                                  a.acceleration.z);

        // Get current orientation quaternion and build rotation matrix
        float qw, qx, qy, qz;
        filters[imu_id].getQuaternion(&qw, &qx, &qy, &qz);
        double R[3][3];
        quaternionToRotationMatrix(qw, qx, qy, qz, R);

        // Rotate accel into world frame and subtract gravity from Z
        double ax = a.acceleration.x;
        double ay = a.acceleration.y;
        double az = a.acceleration.z;
        imu_buffer[window_idx].x = R[0][0] * ax + R[0][1] * ay + R[0][2] * az;
        imu_buffer[window_idx].y = R[1][0] * ax + R[1][1] * ay + R[1][2] * az;
        imu_buffer[window_idx].z =
            R[2][0] * ax + R[2][1] * ay + R[2][2] * az - calib_offsets.g_mag;

        // World-frame angular velocity (rad/s)
        double gx = g.gyro.x, gy = g.gyro.y, gz = g.gyro.z;
        imu_buffer[window_idx].roll =
            R[0][0] * gx + R[0][1] * gy + R[0][2] * gz;
        imu_buffer[window_idx].pitch =
            R[1][0] * gx + R[1][1] * gy + R[1][2] * gz;
        imu_buffer[window_idx].yaw =
            R[2][0] * gx + R[2][1] * gy + R[2][2] * gz;

        // Update indices
        sz = min(sz + 1, SLIDING_WINDOW_SIZE);
        window_idx = (window_idx + 1) % SLIDING_WINDOW_SIZE;

        // Compute sliding window average and send to queue
        imu_reading_t sensor_result = windowAvg(imu_buffer, sz);
        xQueueOverwrite(xIMUQueue[imu_id], &sensor_result);
        vTaskDelay(pdMS_TO_TICKS(IMU_PERIOD_MS));
    }

    // On closing of task, clear IMU initialized bit and delete task (not
    // expected to reach here)
    xEventGroupClearBits(xSystemEventGroup, (1 << imu_id));
    vTaskDelete(NULL);
}