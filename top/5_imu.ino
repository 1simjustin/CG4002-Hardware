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

// Compute rotation matrix R that maps measured gravity vector to Z-up (0,0,1)
// Uses Rodrigues' rotation formula
void computeRotationMatrix(double gx, double gy, double gz, double R[3][3]) {
    double mag = sqrt(gx * gx + gy * gy + gz * gz);

    // Fallback: if no measurable gravity, use identity
    if (mag < 1e-6) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                R[i][j] = (i == j) ? 1.0 : 0.0;
        return;
    }

    // Normalize gravity vector
    double vx = gx / mag;
    double vy = gy / mag;
    double vz = gz / mag;

    // Cross product: v × (0,0,1) = (vy, -vx, 0)
    double s = sqrt(vx * vx + vy * vy); // sin(theta)
    double c = vz;                      // cos(theta)

    if (s < 1e-6) {
        // Gravity is already aligned with Z
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                R[i][j] = 0.0;

        if (c > 0) {
            // Already pointing +Z, identity
            R[0][0] = 1.0;
            R[1][1] = 1.0;
            R[2][2] = 1.0;
        } else {
            // Pointing -Z, 180° rotation about X axis
            R[0][0] = 1.0;
            R[1][1] = -1.0;
            R[2][2] = -1.0;
        }
        return;
    }

    // Normalized rotation axis components (k_z = 0)
    double kx = vy / s;
    double ky = -vx / s;

    double one_minus_c = 1.0 - c;

    R[0][0] = 1.0 - ky * ky * one_minus_c;
    R[0][1] = kx * ky * one_minus_c;
    R[0][2] = ky * s;

    R[1][0] = kx * ky * one_minus_c;
    R[1][1] = 1.0 - kx * kx * one_minus_c;
    R[1][2] = -kx * s;

    R[2][0] = -ky * s;
    R[2][1] = kx * s;
    R[2][2] = c;
}

// Calibrate IMU: compute rotation matrix from gravity and gyro biases
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

    // Rotation matrix: sensor frame -> Z-up reference frame
    computeRotationMatrix(avg_ax, avg_ay, avg_az, calib.R);

    return calib;
}

void applyCalibration(sensors_event_t *a, sensors_event_t *g,
                      imu_calib_t *calib) {
    // Rotate accelerometer into reference frame and subtract gravity from Z
    double ax = a->acceleration.x;
    double ay = a->acceleration.y;
    double az = a->acceleration.z;

    a->acceleration.x =
        calib->R[0][0] * ax + calib->R[0][1] * ay + calib->R[0][2] * az;
    a->acceleration.y =
        calib->R[1][0] * ax + calib->R[1][1] * ay + calib->R[1][2] * az;
    a->acceleration.z = calib->R[2][0] * ax + calib->R[2][1] * ay +
                        calib->R[2][2] * az - calib->g_mag;

    // Subtract gyro bias in sensor frame, then rotate into reference frame
    double gx = g->gyro.x - calib->gyro_bias[0];
    double gy = g->gyro.y - calib->gyro_bias[1];
    double gz = g->gyro.z - calib->gyro_bias[2];

    g->gyro.x = calib->R[0][0] * gx + calib->R[0][1] * gy + calib->R[0][2] * gz;
    g->gyro.y = calib->R[1][0] * gx + calib->R[1][1] * gy + calib->R[1][2] * gz;
    g->gyro.z = calib->R[2][0] * gx + calib->R[2][1] * gy + calib->R[2][2] * gz;
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
    return;
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
#if defined(USE_AHRS)
                filter_setup(imu_id);
#endif
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
            // If not running, wait before checking again
            vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_DELAY_MS));
            continue;
        }

        if (!mpu_devices[imu_id].getEvent(&a, &g, &temp)) {
            vTaskDelay(pdMS_TO_TICKS(IMU_PERIOD_MS));
            continue;
        }
        applyCalibration(&a, &g, &calib_offsets);

        // Store raw readings in buffer for sliding window average
        imu_buffer[window_idx].x = a.acceleration.x;
        imu_buffer[window_idx].y = a.acceleration.y;
        imu_buffer[window_idx].z = a.acceleration.z;

#ifdef USE_AHRS
        filters[imu_id].update(g.gyro.x, g.gyro.y, g.gyro.z, a.acceleration.x,
                               a.acceleration.y,
                               a.acceleration.z + calib_offsets.g_mag // AHRS expects gravity
                               );
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
        vTaskDelay(pdMS_TO_TICKS(IMU_PERIOD_MS));
    }

    // On closing of task, clear IMU initialized bit and delete task (not
    // expected to reach here)
    xEventGroupClearBits(xSystemEventGroup, (1 << imu_id));
    vTaskDelete(NULL);
}