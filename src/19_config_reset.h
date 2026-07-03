// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= CONFIG RESET RUNTIME HANDLER =================
void handleConfigResetButton() {
  // FIX v2.13:
  // External hard reset button is removed from runtime flow.
  // Reset language/config with BTN_PLAYER + BTN_WEATHER hold 5 seconds.
  static unsigned long comboDownAt = 0;
  static bool comboArmed = false;

  bool comboDown = resetComboHeld();

  if (comboDown && comboDownAt == 0) {
    comboDownAt = millis();
    comboArmed = true;
    Serial.println("CONFIG COMBO: BTN1+BTN3 pressed");
  }

  if (!comboDown) {
    comboDownAt = 0;
    comboArmed = false;
    return;
  }

  if (comboArmed && millis() - comboDownAt >= 5000) {
    comboArmed = false;

    Serial.println("CONFIG COMBO: clearing language/config");
    prefs.clear();

    u8g2.clearBuffer();
    centerText("Config reset", 28, u8g2_font_6x10_tr);
    centerText("Select language", 43, u8g2_font_6x10_tr);
    u8g2.sendBuffer();
    delay(700);

    // Wait until both buttons are released, otherwise selection may trigger instantly.
    while (digitalRead(BTN_PLAYER) == LOW || digitalRead(BTN_WEATHER) == LOW) {
      delay(30);
    }

    // Show language selection again without hard reboot.
    prefs.putUInt("cfgVer", 0);
    loadOrSelectLanguage();

    currentScreen = SCREEN_GREETING;
    greetingUntil = millis() + 1800;
    manualScreenLock = false;
    requestRedraw();
  }
}
