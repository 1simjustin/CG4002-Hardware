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
  // Clamp voltage to reference voltage to avoid erroneous readings
  // Especially important since voltage from USB reads as 5V which is above the ADC reference voltage
  if (voltage >= BATT_VOLT_MAX) {
    return 100;
  }
  if (voltage <= BATT_VOLT_MIN) {
    return 0;
  }
  return (int)map(voltage * 100, BATT_VOLT_MIN * 100, BATT_VOLT_MAX * 100, 0, 100);
}

void battTask(void *parameter) {
  for (;;) {
    int adc_value = analogRead(BATTERY_PIN);
    batt_voltage = adc_value * ADC_REF_VOLTAGE * VOLTAGE_DIVIDER_RATIO / ADC_MAX;

    batt_percentage = batt_soc(batt_voltage);

    xSemaphoreGive(xBattSemaphore);
    vTaskDelay(BATT_PERIOD_MS / portTICK_PERIOD_MS);
  }
}

void battDispTask(void *parameter) {
  pinMode(BATT_LED_PIN, OUTPUT);

  for (;;) {
    if (batt_percentage < BATT_CRITICAL_THRESHOLD) {
      blink_state = !blink_state;
      analogWrite(BATT_LED_PIN, blink_state ? 255 : 0);  //
    } else if (batt_percentage < BATT_LOW_THRESHOLD) {
      uint8_t pwm_val = (uint8_t)map(batt_percentage, 0, 50, 255, 0);
      analogWrite(BATT_LED_PIN, pwm_val);
    } else {
      analogWrite(BATT_LED_PIN, 0);  // LED off
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}