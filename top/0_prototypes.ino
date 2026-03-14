/**
 * Generic Function Prototypes
 *
 * I dont know why putting this in header does not work, but it doesnt, so here
 * we are.
 */

#include "definitions.h"
#include "header.h"

// Setup Functions
void i2c_setup();
bool imu_setup(int idx);
void filter_setup(int idx);
void voltage_reader_setup();
void createSemaphores();

// Comms Functions
void serialSensorsTask(void *parameter);
void serialBattTask(void *parameter);
void commsSensorsTask(void *parameter);
void commsBattTask(void *parameter);

void connectWiFi();
void connectMQTT();
void sendSensorPacket(imu_reading_t (*imu_data)[NUM_IMU]);
void callback(char *topic, byte *payload, unsigned int length);

// Sensor Functions
void imuTask(void *parameter);
imu_reading_t calibrate(Adafruit_MPU6050 mpu);
void applyCalibration(sensors_event_t *a, sensors_event_t *g,
                      imu_reading_t *calib);
imu_reading_t windowAvg(imu_reading_t *readings, int size);

// Battery Functions
int batt_soc(double voltage);
void battTask(void *parameter);
void battDispTask(void *parameter);

// Heartbeat Functions
void hbDispTask(void *parameter);

// General Utility Functions
double radToDeg(double rad);
double degToRad(double deg);

#if defined(DEBUG)
void monitorTask(void *parameter);
#endif