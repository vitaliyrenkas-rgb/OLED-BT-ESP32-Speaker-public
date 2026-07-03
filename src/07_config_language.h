// Auto-split from monolithic OLED BT speaker sketch.
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

// ================= CONFIG STORAGE =================
// v3.5-008: storage layer plus runtime wiring. Web portal comes next.
const char* SPEAKER_CONFIG_NAMESPACE = "speaker_cfg";
const uint32_t SPEAKER_CONFIG_VERSION = 1;

String defaultWeatherLocation() {
  String city = String(WEATHER_CITY);
  String country = String(WEATHER_COUNTRY);
  city.trim();
  country.trim();

  if (city.length() == 0) city = "Kyiv";
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
  speakerConfig.btDeviceName = "OLED BT Speaker";
  speakerConfig.welcomeText = "OLED BT Speaker";
  speakerConfig.portalUser = "BTAdmin";
  speakerConfig.portalPass = "BTPassword";
  speakerConfig.loadedFromNvs = false;
}

void normalizeSpeakerConfig() {
  if (speakerConfig.wifiSsid.length() == 0) speakerConfig.wifiSsid = WIFI_SSID;
  if (speakerConfig.weatherApiKey.length() == 0) speakerConfig.weatherApiKey = WEATHER_API_KEY;
  if (speakerConfig.weatherLocation.length() == 0) speakerConfig.weatherLocation = defaultWeatherLocation();
  if (speakerConfig.btDeviceName.length() == 0) speakerConfig.btDeviceName = "OLED BT Speaker";
  if (speakerConfig.welcomeText.length() == 0) speakerConfig.welcomeText = "OLED BT Speaker";
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
