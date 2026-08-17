// OLEG 4.0 Sister: use the stock HU-055 SW1 as a power-state signal.
// External divider: switched MH-M18 VCC -> 91k -> GPIO27 -> 120k -> Lolita GND.
// SW1 OFF is LOW; SW1 ON is HIGH. GPIO27 is RTC-capable on classic ESP32.

#include "esp_sleep.h"
#include "driver/rtc_io.h"

#define SW1_SENSE_PIN 27

const unsigned long SW1_OFF_STABLE_MS = 500UL;

bool sw1LowPending = false;
unsigned long sw1LowSinceMs = 0;

void setupPowerSwitchSense() {
  // ext0 leaves the wake pin routed through RTC IO. Return it to normal GPIO
  // before digitalRead() after a deep-sleep wake.
  rtc_gpio_deinit(GPIO_NUM_27);

  // The external 120k resistor provides the pull-down. Do not enable an
  // internal pull-up or pull-down: it would alter the divider voltage.
  pinMode(SW1_SENSE_PIN, INPUT);
}

bool preparePowerSwitchWakeup() {
  // ext0 is level-sensitive. Switching SW1 back ON raises GPIO27 and wakes
  // the ESP32, which then restarts through setup().
  esp_err_t result = esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, HIGH);
  if (result != ESP_OK) {
    Serial.printf("SW1: failed to configure GPIO27 wakeup (%d)\n", (int)result);
    return false;
  }
  return true;
}

void enterPowerOffDeepSleep(bool oledReady) {
  // Never enter deep sleep without a working wake source.
  if (!preparePowerSwitchWakeup()) return;

  Serial.println("SW1 OFF: OLED off, entering deep sleep");

  if (oledReady) {
    // Activate the SSD1309 controller's power-save mode before ESP32 sleeps.
    u8g2.setPowerSave(1);
    delay(30);
  }

  esp_deep_sleep_start();
}

void handlePowerSwitchAtBoot() {
  if (digitalRead(SW1_SENSE_PIN) == HIGH) return;

  // If the speaker is booted/reset while SW1 is OFF, avoid bringing up the
  // OLED, Wi-Fi, Bluetooth and I2S. Require a stable LOW first.
  const unsigned long lowStartedMs = millis();
  while (digitalRead(SW1_SENSE_PIN) == LOW) {
    if (millis() - lowStartedMs >= SW1_OFF_STABLE_MS) {
      enterPowerOffDeepSleep(false);
      return;
    }
    delay(5);
  }
}

void handlePowerSwitchRuntime() {
  const int level = digitalRead(SW1_SENSE_PIN);
  const unsigned long now = millis();

  if (level == HIGH) {
    sw1LowPending = false;
    return;
  }

  if (!sw1LowPending) {
    sw1LowPending = true;
    sw1LowSinceMs = now;
    return;
  }

  if (now - sw1LowSinceMs < SW1_OFF_STABLE_MS) return;

  // If wake-source setup ever fails, retry only after another full debounce
  // interval instead of flooding Serial on every loop pass.
  sw1LowSinceMs = now;
  enterPowerOffDeepSleep(true);
}
