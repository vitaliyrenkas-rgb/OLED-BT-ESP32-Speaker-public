// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= BUTTONS =================
void setupButtons() {
  pinMode(ADKEY_ADC_PIN, INPUT);
}

int readAdkeyRaw() {
  long sum = 0;

  for (int i = 0; i < 6; i++) {
    sum += analogRead(ADKEY_ADC_PIN);
  }

  return (int)(sum / 6);
}

bool adkeyInRange(int raw, int minRaw, int maxRaw) {
  return raw >= minRaw && raw <= maxRaw;
}

ButtonId buttonFromAdkeyRaw(int raw) {
  if (raw >= ADKEY_NO_BUTTON_RAW) return BUTTON_NONE;
  if (adkeyInRange(raw, ADKEY_PLAYER_MIN_RAW, ADKEY_PLAYER_MAX_RAW)) return BUTTON_PLAYER;
  if (adkeyInRange(raw, ADKEY_CLOCK_MIN_RAW, ADKEY_CLOCK_MAX_RAW)) return BUTTON_CLOCK;
  if (adkeyInRange(raw, ADKEY_WEATHER_MIN_RAW, ADKEY_WEATHER_MAX_RAW)) return BUTTON_WEATHER;
  return BUTTON_NONE;
}

const char* buttonDebugName(ButtonId button) {
  switch (button) {
    case BUTTON_PLAYER: return "PLAYER";
    case BUTTON_CLOCK: return "CLOCK";
    case BUTTON_WEATHER: return "WEATHER";
    case BUTTON_NONE:
    default: return "NONE";
  }
}

void logAdkeyCalibration() {
#if OLEG4_DEBUG_ADKEY
  static unsigned long lastLog = 0;
  unsigned long now = millis();

  if (now - lastLog < ADKEY_DEBUG_LOG_INTERVAL_MS) return;
  lastLog = now;

  int raw = readAdkeyRaw();
  ButtonId decoded = buttonFromAdkeyRaw(raw);

  Serial.print("[ADKEY] raw=");
  Serial.print(raw);
  Serial.print(" decoded=");
  Serial.println(buttonDebugName(decoded));
#endif
}

ButtonId readButtonDown() {
  return buttonFromAdkeyRaw(readAdkeyRaw());
}

bool buttonDown(ButtonId button) {
  return readButtonDown() == button;
}

void openPlayerScreenFromButton() {
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
}

void handleShortButton(ButtonId button) {
  if (currentScreen == SCREEN_GREETING) return;

  lastUserInteractionMs = millis();
  manualScreenLock = true;

  if (button == BUTTON_PLAYER) {
    openPlayerScreenFromButton();
  } else if (button == BUTTON_CLOCK) {
    currentScreen = SCREEN_CLOCK;
  } else if (button == BUTTON_WEATHER) {
    currentScreen = SCREEN_WEATHER;
  } else {
    return;
  }

  requestRedraw();
}

void handleButtons() {
  ButtonId down = readButtonDown();

  static ButtonId activeButton = BUTTON_NONE;
  static unsigned long activeDownAt = 0;
  static unsigned long lastSeenDownAt = 0;
  static bool longActionFired = false;

  unsigned long now = millis();

  if (down != BUTTON_NONE) {
    if (activeButton != down) {
      activeButton = down;
      activeDownAt = now;
      longActionFired = false;
    }

    lastSeenDownAt = now;

    if (activeButton == BUTTON_PLAYER &&
        !longActionFired &&
        currentScreen != SCREEN_GREETING &&
        now - activeDownAt >= BUTTON_RUNTIME_HOLD_MS) {
      longActionFired = true;
      toggleRuntimeLanguage();
    }

    return;
  }

  if (activeButton != BUTTON_NONE) {
    // ADKEY ladders can briefly decode as NONE while a button is still physically held.
    // Do not globally debounce short presses; only give BTN1 long-hold detection a small
    // dropout grace window so the 7-second language action is not reset by ADC noise.
    unsigned long heldSoFar = now - activeDownAt;
    if (!longActionFired &&
        activeButton == BUTTON_PLAYER &&
        heldSoFar >= 500UL &&
        now - lastSeenDownAt <= BUTTON_HOLD_GRACE_MS) {
      return;
    }

    ButtonId releasedButton = activeButton;
    unsigned long heldMs = lastSeenDownAt >= activeDownAt ? (lastSeenDownAt - activeDownAt) : (now - activeDownAt);
    bool consumedByLongAction = longActionFired;

    activeButton = BUTTON_NONE;
    activeDownAt = 0;
    lastSeenDownAt = 0;
    longActionFired = false;

    if (!consumedByLongAction && heldMs >= BUTTON_SHORT_PRESS_MIN_MS) {
      handleShortButton(releasedButton);
    }
  }

  if (currentScreen == SCREEN_MESSAGE && millis() >= messageUntil) {
    currentScreen = returnScreen;
    requestRedraw();
  }
}
