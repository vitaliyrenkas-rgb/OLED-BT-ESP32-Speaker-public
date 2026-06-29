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

void updateBatteryChargingState(float v) {
  static float trendBaseVoltage = 0.0;
  static unsigned long lastTrendCheckMs = 0;
  static uint8_t riseHits = 0;
  static uint8_t fallHits = 0;

  unsigned long now = millis();

  if (v < BATTERY_ABSENT_VOLTAGE) {
    batteryCharging = false;
    trendBaseVoltage = 0.0;
    riseHits = 0;
    fallHits = 0;
    return;
  }

  if (trendBaseVoltage <= 0.0) {
    trendBaseVoltage = v;
    lastTrendCheckMs = now;
    batteryLastChargeRiseMs = now;
    return;
  }

  // updateBattery() is called once per 10s; this keeps one trend sample per call.
  if (now - lastTrendCheckMs < 9000UL) {
    return;
  }

  lastTrendCheckMs = now;
  float delta = v - trendBaseVoltage;

  if (delta >= 0.010f) {
    riseHits++;
    fallHits = 0;
    trendBaseVoltage = v;
    batteryLastChargeRiseMs = now;

    // Two consecutive +10mV steps: charging is very likely.
    if (riseHits >= 2 && v < 4.18f) {
      batteryCharging = true;
    }
  } else if (delta <= -0.020f) {
    fallHits++;
    riseHits = 0;
    trendBaseVoltage = v;

    // Two clear downward steps: charging stopped / running from battery.
    if (fallHits >= 2) {
      batteryCharging = false;
    }
  } else {
    // Small ADC noise / flat voltage. Keep current state for a while, then stop animation.
    if (batteryCharging && now - batteryLastChargeRiseMs > 20UL * 60UL * 1000UL) {
      batteryCharging = false;
    }
  }

  if (v >= 4.18f) {
    batteryCharging = false;
  }

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
  batteryPresent = batteryVoltage >= BATTERY_ABSENT_VOLTAGE;
  batteryPercent = voltageToPercent(batteryVoltage);
  updateBatteryChargingState(batteryVoltage);
}
