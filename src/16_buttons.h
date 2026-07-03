// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= BUTTONS =================
void setupButtons() {
  pinMode(BTN_PLAYER, INPUT_PULLUP);
  pinMode(BTN_CLOCK, INPUT_PULLUP);
  pinMode(BTN_WEATHER, INPUT_PULLUP);
}

bool buttonPressed(int pin) {
  static unsigned long lastP = 0;
  static unsigned long lastC = 0;
  static unsigned long lastW = 0;

  unsigned long *last;

  if (pin == BTN_PLAYER) last = &lastP;
  else if (pin == BTN_CLOCK) last = &lastC;
  else last = &lastW;

  if (digitalRead(pin) == LOW && millis() - *last > 350) {
    *last = millis();
    return true;
  }

  return false;
}

void handleButtons() {
  // FIX v3.0: block normal screen switching while config reset combo is held.
  if (resetComboHeld()) return;

  if (currentScreen == SCREEN_GREETING) return;

  if (buttonPressed(BTN_PLAYER)) {
    lastUserInteractionMs = millis();
    manualScreenLock = true;

    if (currentScreen == SCREEN_SLEEP) {
      currentScreen = SCREEN_CLOCK;
      manualScreenLock = false;
    } else if (btConnected) {
      currentScreen = SCREEN_PLAYER;
    } else {
      returnScreen = SCREEN_CLOCK;
      currentScreen = SCREEN_MESSAGE;
      messageUntil = millis() + 5000;
    }

    requestRedraw();
  }

  if (buttonPressed(BTN_CLOCK)) {
    lastUserInteractionMs = millis();
    manualScreenLock = true;
    currentScreen = SCREEN_CLOCK;
    requestRedraw();
  }

  if (buttonPressed(BTN_WEATHER)) {
    lastUserInteractionMs = millis();
    manualScreenLock = true;
    currentScreen = SCREEN_WEATHER;
    requestRedraw();
  }

  if (currentScreen == SCREEN_MESSAGE && millis() >= messageUntil) {
    currentScreen = returnScreen;
    requestRedraw();
  }
}
