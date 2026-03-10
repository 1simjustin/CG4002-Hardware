/**
 * Upper Limb MPU Addr - 0x68
 * Lower Limb MPU Addr - 0x69
 */

double radToDeg(double rad) {
    double deg = rad * RAD_TO_DEG;
    return deg;
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
        x_out[imu_id] = NULL;
        y_out[imu_id] = NULL;
        z_out[imu_id] = NULL;
        roll_out[imu_id] = NULL;
        pitch_out[imu_id] = NULL;
        yaw_out[imu_id] = NULL;

        vTaskDelete(NULL); // Delete task if IMU setup fails
    }

    filter_setup(imu_id);
    imu_init[imu_id] = true;

    for (;;) {
        mpu_devices[imu_id].getEvent(&a_arr[imu_id], &g_arr[imu_id],
                                     &temp[imu_id]);

        x_out[imu_id] = a_arr[imu_id].acceleration.x;
        y_out[imu_id] = a_arr[imu_id].acceleration.y;
        z_out[imu_id] = a_arr[imu_id].acceleration.z;

#ifdef USE_AHRS
        filters[imu_id].update(
            g_arr[imu_id].gyro.x, g_arr[imu_id].gyro.y, g_arr[imu_id].gyro.z,
            a_arr[imu_id].acceleration.x, a_arr[imu_id].acceleration.y,
            a_arr[imu_id].acceleration.z);
        roll_out[imu_id] = filters[imu_id].getRoll();
        pitch_out[imu_id] = filters[imu_id].getPitch();
        yaw_out[imu_id] = filters[imu_id].getYaw();
#else
        roll_out[imu_id] = radToDeg(g_arr[imu_id].gyro.x);
        pitch_out[imu_id] = radToDeg(g_arr[imu_id].gyro.y);
        yaw_out[imu_id] = radToDeg(g_arr[imu_id].gyro.z);
#endif

        xSemaphoreGive(xIMUSemaphore[imu_id]);
    }
}