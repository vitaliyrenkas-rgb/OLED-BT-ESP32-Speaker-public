// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= CONFIG / LANGUAGE =================
void drawLanguageSelectScreen() {
  u8g2.clearBuffer();
  u8g2.drawFrame(5, 8, 118, 48);

  centerText("Select Language", 24, u8g2_font_6x10_tr);

  // FIX v2.12:
  // Use ASCII "UA" here because some Cyrillic fonts render "Укр." as a dot on this build.
  // Button 1 = left bottom option, button 3 = right bottom option.
  u8g2.setFont(u8g2_font_6x10_tr);

  u8g2.drawStr(12, 51, "En");

  int buildW = u8g2.getStrWidth(BUILD_VERSION);
  u8g2.drawStr((128 - buildW) / 2, 51, BUILD_VERSION);

  u8g2.drawStr(100, 51, "UA");

  u8g2.sendBuffer();
}

bool checkConfigResetAtBoot() {
  // FIX v2.13:
  // External hard reset button disabled.
  // Use BTN_PLAYER + BTN_WEATHER hold 5s during runtime instead.
  return false;
}

void loadOrSelectLanguage() {
  prefs.begin("speaker", false);

  bool resetConfig = checkConfigResetAtBoot();
  if (resetConfig) {
    Serial.println("CONFIG RESET: hard erase NVS at boot");
    prefs.clear();
    prefs.end();
    nvs_flash_erase();
    nvs_flash_init();
    prefs.begin("speaker", false);
  }

  uint32_t storedVersion = prefs.getUInt("cfgVer", 0);
  String storedLang = prefs.getString("lang", "");

  if (storedVersion == CONFIG_VERSION && (storedLang == "en" || storedLang == "ua")) {
    uiLang = storedLang == "ua" ? LANG_UA : LANG_EN;
    Serial.print("Loaded language: ");
    Serial.println(storedLang);
    return;
  }

  drawLanguageSelectScreen();

  // FIX v2.12:
  // Do not block forever on language selection.
  // If no button is pressed, default to UA after 10 seconds.
  unsigned long start = millis();

  while (true) {
    if (digitalRead(BTN_PLAYER) == LOW) {
      uiLang = LANG_EN;
      prefs.putString("lang", "en");
      prefs.putUInt("cfgVer", CONFIG_VERSION);
      Serial.println("Language selected: en");
      break;
    }

    if (digitalRead(BTN_WEATHER) == LOW) {
      uiLang = LANG_UA;
      prefs.putString("lang", "ua");
      prefs.putUInt("cfgVer", CONFIG_VERSION);
      Serial.println("Language selected: ua");
      break;
    }

    if (millis() - start > 10000) {
      uiLang = LANG_UA;
      prefs.putString("lang", "ua");
      prefs.putUInt("cfgVer", CONFIG_VERSION);
      Serial.println("Language timeout: default ua");
      break;
    }

    delay(40);
  }

  u8g2.clearBuffer();
  if (uiLang == LANG_UA) centerText("UA saved", 34, u8g2_font_6x10_tr);
  else centerText("Language saved", 34, u8g2_font_6x10_tr);
  u8g2.sendBuffer();
  delay(800);
}
