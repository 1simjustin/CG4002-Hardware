// MPU_ID 0: Left Arm
// MPU_ID 1: Right Arm
// MPU_ID 2: Chest

double radToDeg(double rad) {
  return rad * RAD_TO_DEG;
}

#ifdef TORSO_DEVICE
void set_mux_channel(int channel) {
  if (channel > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}
#endif

void imu_setup(int idx) {
  #ifdef TORSO_DEVICE
  // Initialize each IMU by switching the Mux to the correct channel
  set_mux_channel(idx);
  #endif

  if (!mpu_devices[idx].begin(MPU6050_DEVICE_ID)) {
    Serial.print("Failed to find MPU6050 chip for IMU ");
    Serial.println(idx);
    while (1) {
      delay(50);
    }
  }

  #ifdef DEBUG
  Serial.print("MPU6050 Found for IMU ");
  Serial.println(idx);
  #endif

  mpu_devices[idx].setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu_devices[idx].setGyroRange(MPU6050_RANGE_500_DEG);
  mpu_devices[idx].setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void filter_setup(int idx) {
  filters[idx].begin(IMU_FREQ_HZ);
}

void imuTask(void *parameter) {
  int imu_id = (int)parameter;
  imu_setup(imu_id);

  for (;;) {
    #ifdef TORSO_DEVICE
    set_mux_channel(imu_id);
    #endif

    mpu_devices[imu_id].getEvent(&a_arr[imu_id], &g_arr[imu_id], &temp[imu_id]);

    x_out[imu_id] = a_arr[imu_id].acceleration.x;
    y_out[imu_id] = a_arr[imu_id].acceleration.y;
    z_out[imu_id] = a_arr[imu_id].acceleration.z;

    #ifdef USE_AHRS
    filters[imu_id].update(g_arr[imu_id].gyro.x, g_arr[imu_id].gyro.y, g_arr[imu_id].gyro.z,
                      a_arr[imu_id].acceleration.x, a_arr[imu_id].acceleration.y, a_arr[imu_id].acceleration.z);
    roll_out[imu_id] = filters[imu_id].getRoll();
    pitch_out[imu_id] = filters[imu_id].getPitch();
    yaw_out[imu_id] = filters[imu_id].getYaw();
    #else
    roll_out[imu_id] = radToDeg(g_arr[imu_id].gyro.x);
    pitch_out[imu_id] = radToDeg(g_arr[imu_id].gyro.y);
    yaw_out[imu_id] = radToDeg(g_arr[imu_id].gyro.z);
    #endif

#if defined(TORSO_DEVICE) || defined(LEG_DEVICE)
    xSemaphoreGive(xIMUSemaphore[imu_id]);
    vTaskDelay(IMU_PERIOD_MS / portTICK_PERIOD_MS);
#endif
  }
}