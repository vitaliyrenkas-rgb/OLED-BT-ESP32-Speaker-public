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

float readUsbVbusVoltage() {
  long sum = 0;

  for (int i = 0; i < 8; i++) {
    sum += analogRead(USB_VBUS_ADC_PIN);
  }

  float raw = sum / 8.0;
  float adcVoltage = (raw / ADC_MAX) * ADC_REF_VOLTAGE;
  return adcVoltage * USB_VBUS_DIVIDER_RATIO;
}

bool readUsbPowerPresent() {
  return readUsbVbusVoltage() >= USB_VBUS_PRESENT_VOLTAGE;
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

void updateBatteryChargingState(float v) {
  // GPIO33 senses USB/VBUS through a 100k/100k divider.
  // GPIO34 still measures the battery itself.
  // Do not infer unplug from battery noise: ADC/BAT can jump by ~30mV.
  if (!usbPowerPresent || v < BATTERY_ABSENT_VOLTAGE || v >= 4.18) {
    batteryCharging = false;
    batteryLastChargeRiseMs = millis();
    return;
  }

  batteryCharging = true;
  batteryLastChargeRiseMs = millis();
}

uint8_t getBatteryIconPercent() {
  uint8_t realPercent = (uint8_t)constrain(batteryPercent, 0, 100);

  static bool animActive = false;
  static uint8_t animPercent = 0;
  static unsigned long lastAnimStepMs = 0;

  unsigned long now = millis();

  if (!batteryCharging) {
    animActive = false;
    animPercent = realPercent;
    lastAnimStepMs = now;
    return realPercent;
  }

  if (!animActive) {
    animActive = true;
    animPercent = realPercent;
    lastAnimStepMs = now;
    return animPercent;
  }

  if (now - lastAnimStepMs >= 450UL) {
    lastAnimStepMs = now;

    if (animPercent >= 100) {
      // New cycle starts from the currently measured charge.
      animPercent = realPercent;
    } else {
      // Forward only: current -> next 20% bucket -> ... -> 100.
      uint8_t nextPercent = ((animPercent / 20) + 1) * 20;

      if (nextPercent > 100) {
        nextPercent = 100;
      }

      // If real charge jumped upward, do not show lower than real.
      if (nextPercent < realPercent) {
        nextPercent = realPercent;
      }

      animPercent = nextPercent;
    }
  }

  return animPercent;
}

void updateBattery() {
  batteryVoltage = readBatteryVoltage();
  usbPowerPresent = readUsbPowerPresent();
  batteryPresent = batteryVoltage >= BATTERY_ABSENT_VOLTAGE;
  batteryPercent = voltageToPercent(batteryVoltage);
  updateBatteryChargingState(batteryVoltage);
}
