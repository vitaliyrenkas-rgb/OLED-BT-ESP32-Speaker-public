// Auto-split from monolithic OLED BT speaker sketch.
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

  // v3.5-010: UI calibration for the actual JBL-style 1S pack/charger path.
  // In this build the measured full/near-full battery voltage tops out around
  // 3.94-4.00V, so the display uses 4.00V as the practical 100% point.
  if (v >= BATTERY_FULL_VOLTAGE) return 100;
  if (v >= 3.94) return 95;
  if (v >= 3.88) return 90;
  if (v >= 3.82) return 80;
  if (v >= 3.76) return 70;
  if (v >= 3.70) return 60;
  if (v >= 3.62) return 45;
  if (v >= 3.55) return 30;
  if (v >= 3.45) return 15;
  if (v >= 3.35) return 5;
  return 0;
}

void updateBatteryChargingState(float v) {
  // GPIO33 senses USB/VBUS through a 100k/100k divider.
  // GPIO34 still measures the battery itself.
  // Do not infer unplug from battery noise: ADC/BAT can jump by ~30mV.
  if (!usbPowerPresent || v < BATTERY_ABSENT_VOLTAGE || v >= BATTERY_FULL_VOLTAGE) {
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

int readVolumePotRaw() {
  long sum = 0;

  for (int i = 0; i < 8; i++) {
    sum += analogRead(VOLUME_ADC_PIN);
  }

  return (int)(sum / 8);
}

int volumeRawToValue(int raw) {
  raw = constrain(raw, 0, (int)ADC_MAX);

  // Snap the ADC edges so the knob can reach exact 0 and 127.
  if (raw <= 20) return VOLUME_POT_MIN_VOLUME;
  if (raw >= 4050) return VOLUME_POT_MAX_VOLUME;

  long volume = map(raw, 0, (long)ADC_MAX, VOLUME_POT_MIN_VOLUME, VOLUME_POT_MAX_VOLUME);
  return constrain((int)volume, VOLUME_POT_MIN_VOLUME, VOLUME_POT_MAX_VOLUME);
}

int volumeValueToOverlayPercent(int volume) {
  long percent = map(volume, VOLUME_POT_MIN_VOLUME, VOLUME_POT_MAX_VOLUME, 0, 100);
  return constrain((int)percent, 0, 100);
}

void showVolumeOverlay(int appliedVolume, unsigned long now) {
  volumeOverlayPercent = volumeValueToOverlayPercent(appliedVolume);
  volumeOverlayLastChangeMs = now;
  volumeOverlayActive = true;
}

void updateVolumeFromPot(unsigned long now) {
  if (lastVolumePotReadMs != 0 && now - lastVolumePotReadMs < VOLUME_POT_UPDATE_INTERVAL) {
    return;
  }

  lastVolumePotReadMs = now;

  int raw = readVolumePotRaw();

  static bool smoothingReady = false;
  static int smoothedRaw = 0;

  if (!smoothingReady) {
    smoothedRaw = raw;
    smoothingReady = true;
  } else {
    int rawDelta = abs(raw - smoothedRaw);

    if (rawDelta <= VOLUME_POT_RAW_DEADBAND) {
      // Ignore small ADC/pot jitter so the overlay and volume do not twitch.
      raw = smoothedRaw;
    } else if (rawDelta >= VOLUME_POT_FAST_MOVE_RAW) {
      // Large knob movement: follow immediately instead of slowly ramping.
      smoothedRaw = raw;
    } else {
      // Keep the original slow-ish filter: lower sensitivity without making the
      // knob faster than the stable baseline.
      smoothedRaw = (smoothedRaw * 3 + raw) / 4;
    }
  }

  int newVolume = volumeRawToValue(smoothedRaw);

  volumePotRaw = smoothedRaw;
  volumePotVolume = newVolume;

  if (!volumePotReady) {
    volumePotReady = true;
    return;
  }

  // Do not touch A2DP volume until Bluetooth is connected.
  // This keeps boot/discovery behavior close to the stable audio baseline.
  if (!btConnected) {
    volumePotAppliedVolume = -1;
    return;
  }

  if (volumePotAppliedVolume < 0) {
    volumePotAppliedVolume = newVolume;
    a2dp_sink.set_volume(newVolume);
    return;
  }

  if (abs(newVolume - volumePotAppliedVolume) >= VOLUME_POT_DEADBAND) {
    volumePotAppliedVolume = newVolume;
    a2dp_sink.set_volume(newVolume);
    showVolumeOverlay(newVolume, now);
    requestRedraw();
  }
}


float smoothBatteryVoltage(float measuredVoltage) {
  static bool filterReady = false;
  static float filteredVoltage = 0.0;

  float delta = measuredVoltage - filteredVoltage;
  if (delta < 0) delta = -delta;

  if (!filterReady || delta >= BATTERY_FILTER_FAST_DELTA) {
    filteredVoltage = measuredVoltage;
    filterReady = true;
  } else {
    // Battery ADC/load noise can jump enough to move the UI by 10-15%.
    // Smooth the displayed voltage; the real battery changes slowly anyway.
    filteredVoltage = (filteredVoltage * 3.0 + measuredVoltage) / 4.0;
  }

  return filteredVoltage;
}

void updateBattery() {
  float measuredVoltage = readBatteryVoltage();
  batteryVoltage = smoothBatteryVoltage(measuredVoltage);
  usbPowerPresent = readUsbPowerPresent();
  batteryPresent = batteryVoltage >= BATTERY_ABSENT_VOLTAGE;
  batteryPercent = voltageToPercent(batteryVoltage);
  updateBatteryChargingState(batteryVoltage);
}
