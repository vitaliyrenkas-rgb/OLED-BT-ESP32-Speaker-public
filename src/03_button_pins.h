// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= HU-055 BUTTONS / MH-M18-STYLE ADKEY =================
// OLEG 4.0 Sister keeps the stock 3-button resistor ladder.
// One physical ADKEY signal is read through ADC1; order is left-to-right.
#define ADKEY_ADC_PIN 35

enum ButtonId {
  BUTTON_NONE = 0,
  BUTTON_PLAYER,
  BUTTON_CLOCK,
  BUTTON_WEATHER
};

// HU-055 stand thresholds after adding external ADKEY pull-up (10k to 3V3).
// Observed raw levels: idle ~= 4095, left/Player ~= 0, middle/Clock ~= 1805, right/Weather ~= 2860.
// 4.0-007: keep measured bench windows; UX fixes live in the button state machine.
const int ADKEY_PLAYER_MIN_RAW  = 0;
const int ADKEY_PLAYER_MAX_RAW  = 180;
const int ADKEY_CLOCK_MIN_RAW   = 1550;
const int ADKEY_CLOCK_MAX_RAW   = 2100;
const int ADKEY_WEATHER_MIN_RAW = 2550;
const int ADKEY_WEATHER_MAX_RAW = 3150;
const int ADKEY_NO_BUTTON_RAW   = 3700;

const unsigned long BUTTON_SHORT_PRESS_MIN_MS = 40UL;
const unsigned long BUTTON_RELEASE_STABLE_MS = 180UL;
const unsigned long BUTTON_HOLD_GRACE_MS = 280UL;
const unsigned long BUTTON_RUNTIME_HOLD_MS = 7000UL;
const unsigned long BUTTON_LANGUAGE_SELECT_TIMEOUT_MS = 15000UL;

ButtonId readButtonDown();
bool buttonDown(ButtonId button);
