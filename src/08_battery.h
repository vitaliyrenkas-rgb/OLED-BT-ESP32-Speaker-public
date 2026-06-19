// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= BATTERY =================
float readBatteryVoltage() {
  long sum = 0;

  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
  }

  float raw = sum / 8.0;
  float adcVoltage = (raw / ADC_MAX) * ADC_REF_VOLTAGE;
  return adcVoltage * DIVIDER_RATIO;
}

int voltageToPercent(float v) {
  if (v < BATTERY_ABSENT_VOLTAGE) return 0;
  if (v >= 4.20) return 100;
  if (v >= 4.10) return 90;
  if (v >= 4.00) return 80;
  if (v >= 3.90) return 70;
  if (v >= 3.80) return 60;
  if (v >= 3.70) return 50;
  if (v >= 3.60) return 35;
  if (v >= 3.50) return 20;
  if (v >= 3.40) return 10;
  if (v >= 3.30) return 5;
  return 0;
}

void updateBattery() {
  batteryVoltage = readBatteryVoltage();
  batteryPresent = batteryVoltage >= BATTERY_ABSENT_VOLTAGE;
  batteryPercent = voltageToPercent(batteryVoltage);
}
