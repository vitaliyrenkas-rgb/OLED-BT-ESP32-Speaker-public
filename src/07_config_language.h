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

// ================= OLEG CONFIG STORAGE =================
// v3.5-007: storage layer only. Runtime wiring and web portal come next.
const char* OLEG_CONFIG_NAMESPACE = "oleg_cfg";
const uint32_t OLEG_CONFIG_VERSION = 1;

String olegDefaultWeatherLocation() {
  String city = String(WEATHER_CITY);
  String country = String(WEATHER_COUNTRY);
  city.trim();
  country.trim();

  if (city.length() == 0) city = "Zhytomyr";
  if (country.length() == 0) country = "UA";

  return city + "," + country;
}

void splitOlegWeatherLocation(const String& location, String& city, String& country) {
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

void setOlegConfigDefaults() {
  olegConfig.wifiSsid = WIFI_SSID;
  olegConfig.wifiPass = WIFI_PASS;
  olegConfig.weatherApiKey = WEATHER_API_KEY;
  olegConfig.weatherLocation = olegDefaultWeatherLocation();
  splitOlegWeatherLocation(olegConfig.weatherLocation, olegConfig.weatherCity, olegConfig.weatherCountry);
  olegConfig.btDeviceName = "Vitalik Speaker LoLin PROD";
  olegConfig.welcomeText = "Віталік! :)";
  olegConfig.portalUser = "BTAdmin";
  olegConfig.portalPass = "BTPassword";
  olegConfig.loadedFromNvs = false;
}

void normalizeOlegConfig() {
  if (olegConfig.wifiSsid.length() == 0) olegConfig.wifiSsid = WIFI_SSID;
  if (olegConfig.weatherApiKey.length() == 0) olegConfig.weatherApiKey = WEATHER_API_KEY;
  if (olegConfig.weatherLocation.length() == 0) olegConfig.weatherLocation = olegDefaultWeatherLocation();
  if (olegConfig.btDeviceName.length() == 0) olegConfig.btDeviceName = "Vitalik Speaker LoLin PROD";
  if (olegConfig.welcomeText.length() == 0) olegConfig.welcomeText = "Віталік! :)";
  if (olegConfig.portalUser.length() == 0) olegConfig.portalUser = "BTAdmin";
  if (olegConfig.portalPass.length() == 0) olegConfig.portalPass = "BTPassword";

  splitOlegWeatherLocation(olegConfig.weatherLocation, olegConfig.weatherCity, olegConfig.weatherCountry);
}

bool loadOlegConfig() {
  setOlegConfigDefaults();

  Preferences cfgPrefs;
  if (!cfgPrefs.begin(OLEG_CONFIG_NAMESPACE, false)) {
    Serial.println("OLEG config: NVS open failed, using defaults");
    normalizeOlegConfig();
    return false;
  }

  uint32_t storedVersion = cfgPrefs.getUInt("cfgVer", 0);
  if (storedVersion == OLEG_CONFIG_VERSION) {
    olegConfig.wifiSsid = cfgPrefs.getString("wifiSsid", olegConfig.wifiSsid);
    olegConfig.wifiPass = cfgPrefs.getString("wifiPass", olegConfig.wifiPass);
    olegConfig.weatherApiKey = cfgPrefs.getString("weatherKey", olegConfig.weatherApiKey);
    olegConfig.weatherLocation = cfgPrefs.getString("weatherLoc", olegConfig.weatherLocation);
    olegConfig.btDeviceName = cfgPrefs.getString("btName", olegConfig.btDeviceName);
    olegConfig.welcomeText = cfgPrefs.getString("welcome", olegConfig.welcomeText);
    olegConfig.portalUser = cfgPrefs.getString("portalUser", olegConfig.portalUser);
    olegConfig.portalPass = cfgPrefs.getString("portalPass", olegConfig.portalPass);
    olegConfig.loadedFromNvs = true;
  }

  cfgPrefs.end();
  normalizeOlegConfig();

  Serial.print("OLEG config: ");
  Serial.println(olegConfig.loadedFromNvs ? "loaded from NVS" : "defaults");
  return olegConfig.loadedFromNvs;
}

bool saveOlegConfig() {
  normalizeOlegConfig();

  Preferences cfgPrefs;
  if (!cfgPrefs.begin(OLEG_CONFIG_NAMESPACE, false)) {
    Serial.println("OLEG config: NVS open failed, save skipped");
    return false;
  }

  cfgPrefs.putUInt("cfgVer", OLEG_CONFIG_VERSION);
  cfgPrefs.putString("wifiSsid", olegConfig.wifiSsid);
  cfgPrefs.putString("wifiPass", olegConfig.wifiPass);
  cfgPrefs.putString("weatherKey", olegConfig.weatherApiKey);
  cfgPrefs.putString("weatherLoc", olegConfig.weatherLocation);
  cfgPrefs.putString("btName", olegConfig.btDeviceName);
  cfgPrefs.putString("welcome", olegConfig.welcomeText);
  cfgPrefs.putString("portalUser", olegConfig.portalUser);
  cfgPrefs.putString("portalPass", olegConfig.portalPass);
  cfgPrefs.end();

  olegConfig.loadedFromNvs = true;
  Serial.println("OLEG config: saved to NVS");
  return true;
}

void resetOlegConfigToDefaults(bool saveDefaults = false) {
  Preferences cfgPrefs;
  if (cfgPrefs.begin(OLEG_CONFIG_NAMESPACE, false)) {
    cfgPrefs.clear();
    cfgPrefs.end();
  }

  setOlegConfigDefaults();
  normalizeOlegConfig();

  if (saveDefaults) {
    saveOlegConfig();
  } else {
    Serial.println("OLEG config: reset to defaults in RAM");
  }
}
