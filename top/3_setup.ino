void i2c_setup() {
    Wire.begin();
    Wire.setClock(400000);
}

void imu_setup() {
    #ifdef TORSO_DEVICE
    #endif

    #ifdef LEG_DEVICE
    #endif
}