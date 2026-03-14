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
void calibrate(int idx, calib_data_t *calib_data) {
    sensors_event_t a_samples[CALIBRATION_SAMPLES];
    sensors_event_t g_samples[CALIBRATION_SAMPLES];
    sensors_event_t t_samples[CALIBRATION_SAMPLES];

    // Collect samples for calibration
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        mpu_devices[idx].getEvent(&a_samples[i], &g_samples[i], &t_samples[i]);
        delay(CALIBRATION_DELAY_MS);
    }

    // Compute average for each sensor
    sensors_event_t a_avg = {0}, g_avg = {0};
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        a_avg.acceleration.x += a_samples[i].acceleration.x;
        a_avg.acceleration.y += a_samples[i].acceleration.y;
        a_avg.acceleration.z += a_samples[i].acceleration.z;

        g_avg.gyro.x += g_samples[i].gyro.x;
        g_avg.gyro.y += g_samples[i].gyro.y;
        g_avg.gyro.z += g_samples[i].gyro.z;
    }
    a_avg.acceleration.x /= CALIBRATION_SAMPLES;
    a_avg.acceleration.y /= CALIBRATION_SAMPLES;
    a_avg.acceleration.z /= CALIBRATION_SAMPLES;
    g_avg.gyro.x /= CALIBRATION_SAMPLES;
    g_avg.gyro.y /= CALIBRATION_SAMPLES;
    g_avg.gyro.z /= CALIBRATION_SAMPLES;

    *calib_data = (calib_data_t){a_avg, g_avg};
}

void applyCalibration(sensors_event_t *a, sensors_event_t *g,
                      calib_data_t *calib) {
    a->acceleration.x -= calib->acceleration.acceleration.x;
    a->acceleration.y -= calib->acceleration.acceleration.y;
    a->acceleration.z -= calib->acceleration.acceleration.z;

    g->gyro.x -= calib->gyro.gyro.x;
    g->gyro.y -= calib->gyro.gyro.y;
    g->gyro.z -= calib->gyro.gyro.z;
}

void windowAvg(int idx, int size) {
    double sum_ax = 0, sum_ay = 0, sum_az = 0;
    double sum_gx = 0, sum_gy = 0, sum_gz = 0;

    for (int i = 0; i < size; i++) {
        sum_ax += a_arr[idx][i].acceleration.x;
        sum_ay += a_arr[idx][i].acceleration.y;
        sum_az += a_arr[idx][i].acceleration.z;

#if defined(USE_AHRS)
        sum_gx += roll_out[idx][i];
        sum_gy += pitch_out[idx][i];
        sum_gz += yaw_out[idx][i];
#else
        sum_gx += g_arr[idx][i].gyro.x;
        sum_gy += g_arr[idx][i].gyro.y;
        sum_gz += g_arr[idx][i].gyro.z;
#endif
    }

    data_out[idx].avg_x = sum_ax / size;
    data_out[idx].avg_y = sum_ay / size;
    data_out[idx].avg_z = sum_az / size;
    data_out[idx].avg_roll = sum_gx / size;
    data_out[idx].avg_pitch = sum_gy / size;
    data_out[idx].avg_yaw = sum_gz / size;
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

void imuTask(void *parameter) {
    int imu_id = (int)parameter;
    bool imu_success = imu_setup(imu_id);
    if (!imu_success) {
        imu_init[imu_id] = false;
        vTaskDelete(NULL); // Delete task if IMU setup fails
    }

    filter_setup(imu_id);
    imu_init[imu_id] = true;

    calib_data_t calib_data;
    calibrate(imu_id, &calib_data);
    int window_idx = 0;
    int sz = 0;

#if defined(DEBUG)
    Serial.print("Calibration complete for IMU ");
    Serial.println(imu_id);
#endif

    for (;;) {
        mpu_devices[imu_id].getEvent(&a_arr[imu_id][window_idx],
                                     &g_arr[imu_id][window_idx],
                                     &temp[imu_id][window_idx]);

        applyCalibration(&a_arr[imu_id][window_idx], &g_arr[imu_id][window_idx],
                         &calib_data);
        sz = min(sz + 1, SLIDING_WINDOW_SIZE);

#ifdef USE_AHRS
        filters[imu_id].update(g_arr[imu_id][window_idx].gyro.x,
                               g_arr[imu_id][window_idx].gyro.y,
                               g_arr[imu_id][window_idx].gyro.z,
                               a_arr[imu_id][window_idx].acceleration.x,
                               a_arr[imu_id][window_idx].acceleration.y,
                               a_arr[imu_id][window_idx].acceleration.z);
        roll_out[imu_id][window_idx] = filters[imu_id].getRoll();
        pitch_out[imu_id][window_idx] = filters[imu_id].getPitch();
        yaw_out[imu_id][window_idx] = filters[imu_id].getYaw();
#endif

        windowAvg(imu_id, sz);
        window_idx = (window_idx + 1) % SLIDING_WINDOW_SIZE;
        xSemaphoreGive(xIMUSemaphore[imu_id]);
        vTaskDelay(pdMS_TO_TICKS(1000 / IMU_FREQ_HZ));
    }
}