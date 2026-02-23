void i2c_setup() {
    Wire.begin();
    Wire.setClock(400000);
}

void imu_setup() {
    #ifdef TORSO_DEVICE
    // Setup I2C Mux
    pinMode(I2C_ADDR_SEL_PIN1, OUTPUT);
    pinMode(I2C_ADDR_SEL_PIN2, OUTPUT);

    // Initialize each IMU by switching the Mux to the correct channel
    for (int i = 0; i < NUM_IMUS; i++) {
        // Set Mux to select the correct IMU
        digitalWrite(I2C_ADDR_SEL_PIN1, (i & 0x01) ? HIGH : LOW);
        digitalWrite(I2C_ADDR_SEL_PIN2, (i & 0x02) ? HIGH : LOW);
        delay(10); // Short delay to allow Mux to switch

        if (!mpu[i].begin(0x68, 0x70)) {
            Serial.print("Failed to find MPU6050 chip for IMU ");
            Serial.println(i);
            while (1) {
                delay(50);
            }
        }
        Serial.print("MPU6050 Found for IMU ");
        Serial.println(i);

        mpu[i].setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu[i].setGyroRange(MPU6050_RANGE_500_DEG);
        mpu[i].setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    // After initialization, set Mux back to default (IMU 0)
    digitalWrite(I2C_ADDR_SEL_PIN1, LOW);
    digitalWrite(I2C_ADDR_SEL_PIN2, LOW);
    #endif

    #ifdef LEG_DEVICE
    if (!mpu.begin(MPU6050_DEVICE_ID1, 0x70)) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(50);
        }
    }
    Serial.println("MPU6050 Found!");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    #endif
}

void flex_setup() {
    #ifdef TORSO_DEVICE
    pinMode(FLEX_PIN1, INPUT);
    pinMode(FLEX_PIN2, INPUT);
    #endif

    #ifdef LEG_DEVICE
    pinMode(FLEX_PIN, INPUT);
    #endif
}

void voltage_reader_setup() {
    pinMode(BATTERY_PIN, INPUT);
}