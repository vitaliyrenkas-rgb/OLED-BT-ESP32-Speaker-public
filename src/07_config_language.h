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
  // OLEG 4.0 Sister: no boot-time config reset combo on ADKEY.
  return false;
}

void showLanguageSavedScreen() {
  u8g2.clearBuffer();
  u8g2.drawFrame(8, 12, 112, 40);
  if (uiLang == LANG_UA) centerText("UA saved", 35, u8g2_font_6x10_tr);
  else centerText("EN saved", 35, u8g2_font_6x10_tr);
  u8g2.sendBuffer();
  delay(650);
}

void showLanguageUnchangedScreen() {
  u8g2.clearBuffer();
  u8g2.drawFrame(8, 12, 112, 40);
  centerText("No change", 35, u8g2_font_6x10_tr);
  u8g2.sendBuffer();
  delay(650);
}

void waitForAdkeyRelease(unsigned long stableMs = BUTTON_RELEASE_STABLE_MS,
                         unsigned long timeoutMs = 5000UL) {
  unsigned long start = millis();
  unsigned long releasedSince = 0;

  while (millis() - start < timeoutMs) {
    if (readButtonDown() == BUTTON_NONE) {
      if (releasedSince == 0) releasedSince = millis();
      if (millis() - releasedSince >= stableMs) return;
    } else {
      releasedSince = 0;
    }

    delay(20);
  }
}

ButtonId waitForLanguageChoice(unsigned long timeoutMs, bool defaultUaOnTimeout) {
  unsigned long start = millis();
  ButtonId pressedChoice = BUTTON_NONE;
  unsigned long pressedAt = 0;

  while (true) {
    ButtonId down = readButtonDown();
    bool isChoice = (down == BUTTON_PLAYER || down == BUTTON_WEATHER);

    if (isChoice) {
      if (pressedChoice != down) {
        pressedChoice = down;
        pressedAt = millis();
      }
    } else {
      if (pressedChoice != BUTTON_NONE && millis() - pressedAt >= BUTTON_SHORT_PRESS_MIN_MS) {
        return pressedChoice;
      }
      pressedChoice = BUTTON_NONE;
      pressedAt = 0;
    }

    if (timeoutMs > 0 && millis() - start > timeoutMs) {
      return defaultUaOnTimeout ? BUTTON_WEATHER : BUTTON_NONE;
    }

    delay(30);
  }
}

void saveLanguagePreference() {
  prefs.putString("lang", uiLang == LANG_UA ? "ua" : "en");
  prefs.putUInt("cfgVer", CONFIG_VERSION);
}

bool selectLanguageFromMenu(bool waitReleaseFirst,
                            unsigned long timeoutMs,
                            bool defaultUaOnTimeout,
                            const char* source) {
  drawLanguageSelectScreen();

  // Critical for ADKEY ladders: a long hold that opens this screen must not also
  // auto-select the left option. Selection is accepted only after a fresh press+release.
  if (waitReleaseFirst) {
    Serial.println("Language menu: waiting for button release");
    waitForAdkeyRelease();
  }

  ButtonId choice = waitForLanguageChoice(timeoutMs, defaultUaOnTimeout);
  if (choice == BUTTON_NONE) {
    Serial.print("Language menu timeout/no choice: ");
    Serial.println(source);
    showLanguageUnchangedScreen();
    return false;
  }

  uiLang = (choice == BUTTON_WEATHER) ? LANG_UA : LANG_EN;
  saveLanguagePreference();

  Serial.print("Language selected ");
  Serial.print(uiLang == LANG_UA ? "ua" : "en");
  Serial.print(" from ");
  Serial.println(source);

  showLanguageSavedScreen();
  return true;
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

  // First boot: do not block forever. If nobody chooses, default to UA after 10s.
  selectLanguageFromMenu(false, 10000UL, true, "first boot");
}

void toggleRuntimeLanguage() {
  // Historical name kept for call sites. New behavior is safer:
  // long BTN1 opens language select and waits for release before accepting anything.
  selectLanguageFromMenu(true, BUTTON_LANGUAGE_SELECT_TIMEOUT_MS, false, "runtime BTN1 hold");

  lastUserInteractionMs = millis();
  manualScreenLock = false;
  requestRedraw();
}

// ================= CONFIG STORAGE =================
// v3.5-008: storage layer plus runtime wiring. Web portal comes next.
const char* SPEAKER_CONFIG_NAMESPACE = "speaker_cfg";
const uint32_t SPEAKER_CONFIG_VERSION = 1;

String defaultWeatherLocation() {
  String city = String(WEATHER_CITY);
  String country = String(WEATHER_COUNTRY);
  city.trim();
  country.trim();

  if (city.length() == 0) city = "Zhytomyr";
  if (country.length() == 0) country = "UA";

  return city + "," + country;
}

void splitWeatherLocation(const String& location, String& city, String& country) {
  String value = location;
  value.trim();

  int comma = value.indexOf(',');
  if (comma >= 0) {
    city = value.substring(0, comma);
    country = value.substring(comma + 1);
  } else {
    city = value;
    country = WEATHER_COUNTRY;
  }

  city.trim();
  country.trim();

  if (city.length() == 0) city = WEATHER_CITY;
  if (country.length() == 0) country = WEATHER_COUNTRY;
}

void setSpeakerConfigDefaults() {
  speakerConfig.wifiSsid = WIFI_SSID;
  speakerConfig.wifiPass = WIFI_PASS;
  speakerConfig.weatherApiKey = WEATHER_API_KEY;
  speakerConfig.weatherLocation = defaultWeatherLocation();
  splitWeatherLocation(speakerConfig.weatherLocation, speakerConfig.weatherCity, speakerConfig.weatherCountry);
  speakerConfig.btDeviceName = "Vitalik Speaker LoLin PROD";
  speakerConfig.welcomeText = "Віталік! :)";
  speakerConfig.portalUser = "BTAdmin";
  speakerConfig.portalPass = "BTPassword";
  speakerConfig.loadedFromNvs = false;
}

void normalizeSpeakerConfig() {
  if (speakerConfig.wifiSsid.length() == 0) speakerConfig.wifiSsid = WIFI_SSID;
  if (speakerConfig.weatherApiKey.length() == 0) speakerConfig.weatherApiKey = WEATHER_API_KEY;
  if (speakerConfig.weatherLocation.length() == 0) speakerConfig.weatherLocation = defaultWeatherLocation();
  if (speakerConfig.btDeviceName.length() == 0) speakerConfig.btDeviceName = "Vitalik Speaker LoLin PROD";
  if (speakerConfig.welcomeText.length() == 0) speakerConfig.welcomeText = "Віталік! :)";
  if (speakerConfig.portalUser.length() == 0) speakerConfig.portalUser = "BTAdmin";
  if (speakerConfig.portalPass.length() == 0) speakerConfig.portalPass = "BTPassword";

  splitWeatherLocation(speakerConfig.weatherLocation, speakerConfig.weatherCity, speakerConfig.weatherCountry);
}

bool loadSpeakerConfig() {
  setSpeakerConfigDefaults();

  Preferences cfgPrefs;
  if (!cfgPrefs.begin(SPEAKER_CONFIG_NAMESPACE, false)) {
    Serial.println("Config: NVS open failed, using defaults");
    normalizeSpeakerConfig();
    return false;
  }

  uint32_t storedVersion = cfgPrefs.getUInt("cfgVer", 0);
  if (storedVersion == SPEAKER_CONFIG_VERSION) {
    speakerConfig.wifiSsid = cfgPrefs.getString("wifiSsid", speakerConfig.wifiSsid);
    speakerConfig.wifiPass = cfgPrefs.getString("wifiPass", speakerConfig.wifiPass);
    speakerConfig.weatherApiKey = cfgPrefs.getString("weatherKey", speakerConfig.weatherApiKey);
    speakerConfig.weatherLocation = cfgPrefs.getString("weatherLoc", speakerConfig.weatherLocation);
    speakerConfig.btDeviceName = cfgPrefs.getString("btName", speakerConfig.btDeviceName);
    speakerConfig.welcomeText = cfgPrefs.getString("welcome", speakerConfig.welcomeText);
    speakerConfig.portalUser = cfgPrefs.getString("portalUser", speakerConfig.portalUser);
    speakerConfig.portalPass = cfgPrefs.getString("portalPass", speakerConfig.portalPass);
    speakerConfig.loadedFromNvs = true;
  }

  cfgPrefs.end();
  normalizeSpeakerConfig();

  Serial.print("Config: ");
  Serial.println(speakerConfig.loadedFromNvs ? "loaded from NVS" : "defaults");
  Serial.print("Config BT name: ");
  Serial.println(speakerConfig.btDeviceName);
  Serial.print("Config weather location: ");
  Serial.println(speakerConfig.weatherLocation);
  return speakerConfig.loadedFromNvs;
}

bool saveSpeakerConfig() {
  normalizeSpeakerConfig();

  Preferences cfgPrefs;
  if (!cfgPrefs.begin(SPEAKER_CONFIG_NAMESPACE, false)) {
    Serial.println("Config: NVS open failed, save skipped");
    return false;
  }

  cfgPrefs.putUInt("cfgVer", SPEAKER_CONFIG_VERSION);
  cfgPrefs.putString("wifiSsid", speakerConfig.wifiSsid);
  cfgPrefs.putString("wifiPass", speakerConfig.wifiPass);
  cfgPrefs.putString("weatherKey", speakerConfig.weatherApiKey);
  cfgPrefs.putString("weatherLoc", speakerConfig.weatherLocation);
  cfgPrefs.putString("btName", speakerConfig.btDeviceName);
  cfgPrefs.putString("welcome", speakerConfig.welcomeText);
  cfgPrefs.putString("portalUser", speakerConfig.portalUser);
  cfgPrefs.putString("portalPass", speakerConfig.portalPass);
  cfgPrefs.end();

  speakerConfig.loadedFromNvs = true;
  Serial.println("Config: saved to NVS");
  return true;
}

void resetSpeakerConfigToDefaults(bool saveDefaults = false) {
  Preferences cfgPrefs;
  if (cfgPrefs.begin(SPEAKER_CONFIG_NAMESPACE, false)) {
    cfgPrefs.clear();
    cfgPrefs.end();
  }

  setSpeakerConfigDefaults();
  normalizeSpeakerConfig();

  if (saveDefaults) {
    saveSpeakerConfig();
  } else {
    Serial.println("Config: reset to defaults in RAM");
  }
}
