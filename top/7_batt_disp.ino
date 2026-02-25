/**
 * LED Bar graph only exists on torso device.
 * It is used to show the battery level of the device.
 * By right it should aggregate the battery level of all devices.
 * But at midterm stage we assume no communication between devices
 * We just show the battery level of the torso device for now.
 */

// Battery voltage is read as an analog value and converted to actual voltage using a voltage divider
// We use a 1:1 voltage divider for simplicity

void voltage_reader_setup() {
    pinMode(BATTERY_PIN, INPUT);
}

int batt_soc(double voltage) {
    if (voltage >= BATT_VOLT_MAX) {
        return 100;
    } 
    if (voltage <= BATT_VOLT_MIN) {
        return 0;
    } 
    return (int)map(voltage*100, BATT_VOLT_MIN*100, BATT_VOLT_MAX*100, 0, 100);
}

void battTask(void *parameter) {
  for (;;) {
    int adc_value = analogRead(BATTERY_PIN);
    batt_voltage = (adc_value / ADC_MAX) * ADC_REF_VOLTAGE * VOLTAGE_DIVIDER_RATIO;

    // Clamp voltage to reference voltage to avoid erroneous readings
    // Especially important since voltage from USB reads as 5V which is above the ADC reference voltage
    batt_voltage = max(0.0, min(batt_voltage, ADC_REF_VOLTAGE));
    batt_percentage = batt_soc(batt_voltage);

    xSemaphoreGive(xBattSemaphore);
    vTaskDelay(BATT_PERIOD_MS / portTICK_PERIOD_MS);
  }
}