// MPU_ID 0: Left Arm
// MPU_ID 1: Right Arm
// MPU_ID 2: Chest

double radToDeg(double rad) {
  return rad * RAD_TO_DEG;
}

#ifdef TORSO_DEVICE
void set_mux_channel(int channel) {
  digitalWrite(I2C_ADDR_SEL_PIN1, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(I2C_ADDR_SEL_PIN2, (channel & 0x02) ? HIGH : LOW);
  delay(10); // Short delay to allow Mux to switch
}
#endif

void imu_setup() {
    #ifdef TORSO_DEVICE
    // Setup I2C Mux
    pinMode(I2C_ADDR_SEL_PIN1, OUTPUT);
    pinMode(I2C_ADDR_SEL_PIN2, OUTPUT);

    // Initialize each IMU by switching the Mux to the correct channel
    for (int i = 0; i < NUM_IMUS; i++) {
        set_mux_channel(i);
        if (!mpu_devices[i].begin(MPU6050_DEVICE_ID, 0x70)) {
            Serial.print("Failed to find MPU6050 chip for IMU ");
            Serial.println(i);
            while (1) {
                delay(50);
            }
        }
        Serial.print("MPU6050 Found for IMU ");
        Serial.println(i);

        mpu_devices[i].setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu_devices[i].setGyroRange(MPU6050_RANGE_500_DEG);
        mpu_devices[i].setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    // After initialization, set Mux back to default (IMU 0)
    set_mux_channel(0);
    #endif

    #ifdef LEG_DEVICE
    if (!mpu_devices[0].begin(MPU6050_DEVICE_ID, 0x70)) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(50);
        }
    }
    Serial.println("MPU6050 Found!");

    mpu_devices[0].setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu_devices[0].setGyroRange(MPU6050_RANGE_500_DEG);
    mpu_devices[0].setFilterBandwidth(MPU6050_BAND_21_HZ);
    #endif
}

void imuTask(void *parameter) {
  int imu_id = (int)parameter;
  for (;;) {
    #ifdef TORSO_DEVICE
    set_mux_channel(imu_id);
    #endif

    mpu_devices[imu_id].getEvent(&a_arr[imu_id], &g_arr[imu_id], NULL);

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