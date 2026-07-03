// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= WIFI / TIME / WEATHER =================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);

  // Important for ESP32 Wi-Fi + Bluetooth coexistence.
  // Do NOT use WiFi.setSleep(false) with active Bluetooth A2DP.
  WiFi.setSleep(true);

  if (speakerConfig.wifiSsid.length() == 0) {
    Serial.println("WiFi skipped: empty SSID");
    return;
  }

  WiFi.begin(speakerConfig.wifiSsid.c_str(), speakerConfig.wifiPass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(200);
  }
}

void setupTimeOnce() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NTP skipped: WiFi not connected");
    ntpSynced = false;
    return;
  }

  if (ntpConfigured && ntpSynced) return;

  Serial.println("Configuring NTP...");
  configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
  ntpConfigured = true;

  struct tm t;
  unsigned long start = millis();
  ntpSynced = false;

  while (millis() - start < 15000) {
    if (getLocalTime(&t, 1000) && t.tm_year >= (2024 - 1900)) {
      ntpSynced = true;
      break;
    }

    Serial.println("Waiting for NTP...");
    delay(100);
  }

  if (ntpSynced) Serial.println("NTP synced");
  else Serial.println("NTP failed: time not synced");
}

void updateWeatherFromAPI() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  String weatherCity = speakerConfig.weatherCity;
  String weatherCountry = speakerConfig.weatherCountry;
  String weatherApiKey = speakerConfig.weatherApiKey;
  weatherCity.trim();
  weatherCountry.trim();
  weatherApiKey.trim();

  if (weatherCity.length() == 0 || weatherApiKey.length() == 0) {
    Serial.println("Weather skipped: missing city/API key");
    return;
  }

  String weatherQuery = weatherCity;
  if (weatherCountry.length() > 0) {
    weatherQuery += ",";
    weatherQuery += weatherCountry;
  }

  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               weatherQuery +
               "&appid=" + weatherApiKey +
               "&units=metric&lang=ua";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();

    StaticJsonDocument<1536> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      weatherTemp = doc["main"]["temp"] | 0.0;
      weatherFeels = doc["main"]["feels_like"] | 0.0;
      weatherHumidity = doc["main"]["humidity"] | -1;
      if (weatherHumidity < 0 || weatherHumidity > 100) weatherHumidity = -1;

      int weatherId = doc["weather"][0]["id"] | 800;
      const char* desc = doc["weather"][0]["description"] | "погода";
      String weatherIconCode = String(doc["weather"][0]["icon"] | "01d");
      weatherDesc = String(desc);
      weatherIsNight = weatherIconCode.endsWith("n");

      if (weatherId >= 200 && weatherId < 600) weatherState = "RAIN";
      else if (weatherId >= 600 && weatherId < 700) weatherState = "SNOW";
      else if (weatherId >= 801 && weatherId < 900) weatherState = "CLOUD";
      else weatherState = "SUN";
    }
  }

  http.end();
}

void updateWeatherCycle() {
  Serial.println("WiFi/NTP/weather startup sync...");

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    setupTimeOnce();
    updateWeatherFromAPI();
    wifiLastSyncOk = ntpSynced;
  } else {
    wifiLastSyncOk = false;
  }

  // Critical: Bluetooth A2DP must run without active Wi-Fi in this build.
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(400);

  lastWeatherUpdate = millis();

  Serial.print("WiFi sync result: ");
  Serial.println(wifiLastSyncOk ? "OK" : "FAIL");
}


// ================= CONFIG PORTAL =================
// v3.5-009: AP/local web UI only. Do not keep Wi-Fi active during A2DP audio.
const char* CONFIG_PORTAL_AP_SSID = "OLED-SETUP";
const char* CONFIG_PORTAL_URL = "http://192.168.4.1";
const unsigned long CONFIG_PORTAL_BOOT_HOLD_MS = 1200;

bool configPortalRequestedAtBoot() {
  if (digitalRead(BTN_CLOCK) != LOW) return false;

  unsigned long start = millis();
  while (millis() - start < CONFIG_PORTAL_BOOT_HOLD_MS) {
    if (digitalRead(BTN_CLOCK) != LOW) return false;
    delay(20);
  }

  return true;
}

String htmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}

void appendPortalInput(String& html, const char* label, const char* name, const String& value, const char* type = "text", const char* extra = "") {
  html += "<label>";
  html += label;
  html += "<input type=\"";
  html += type;
  html += "\" name=\"";
  html += name;
  html += "\" value=\"";
  html += htmlEscape(value);
  html += "\"";
  if (extra[0] != '\0') {
    html += " ";
    html += extra;
  }
  html += "></label>";
}

String buildConfigPortalPage(const String& notice = "") {
  String html;
  html.reserve(6500);

  html += F("<!doctype html><html lang=\"uk\"><head><meta charset=\"utf-8\">");
  html += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
  html += F("<title>OLED Config Portal</title>");
  html += F("<style>");
  html += F("body{margin:0;background:#111;color:#eee;font-family:Arial,sans-serif;}main{max-width:520px;margin:0 auto;padding:18px;}");
  html += F("h1{font-size:24px;margin:8px 0 4px}.card{background:#1d1d1d;border:1px solid #333;border-radius:14px;padding:16px;box-shadow:0 0 18px #0006}");
  html += F("label{display:block;margin:13px 0 5px;font-size:14px;color:#ccc}input{box-sizing:border-box;width:100%;margin-top:5px;padding:11px;border-radius:9px;border:1px solid #555;background:#090909;color:#fff;font-size:16px}");
  html += F("button{width:100%;margin-top:18px;padding:12px;border:0;border-radius:10px;background:#f5c542;color:#111;font-weight:700;font-size:16px}.hint{font-size:13px;color:#aaa;line-height:1.35}.ok{background:#173b25;border:1px solid #2d7a48;color:#b9f7c8;padding:10px;border-radius:9px}.info{display:inline-block;margin-left:6px;color:#f5c542;text-decoration:none;font-weight:700}.small{font-size:12px;color:#888}");
  html += F("</style></head><body><main><h1>OLED Config Portal</h1>");
  html += F("<p class=\"hint\">AP: <b>OLED-SETUP</b><br>URL: <b>http://192.168.4.1</b></p>");

  if (notice.length() > 0) {
    html += "<p class=\"ok\">";
    html += htmlEscape(notice);
    html += "</p>";
  }

  html += F("<form class=\"card\" method=\"POST\" action=\"/save\">");
  appendPortalInput(html, "Wi-Fi SSID", "wifiSsid", speakerConfig.wifiSsid);
  appendPortalInput(html, "Wi-Fi password", "wifiPass", speakerConfig.wifiPass, "password");

  html += F("<label>Weather API Key <a class=\"info\" href=\"https://openweathermap.org/appid\" target=\"_blank\" rel=\"noopener\">ⓘ</a>");
  html += "<input type=\"password\" name=\"weatherKey\" value=\"";
  html += htmlEscape(speakerConfig.weatherApiKey);
  html += F("\"><span class=\"small\">OpenWeather API key guide</span></label>");

  appendPortalInput(html, "Location", "weatherLoc", speakerConfig.weatherLocation, "text", "list=\"uaCities\"");

  // Ukrainian regional centers + Kyiv + Simferopol.
  html += F("<datalist id=\"uaCities\">");
  html += F("<option value=\"Kyiv,UA\"><option value=\"Vinnytsia,UA\"><option value=\"Lutsk,UA\"><option value=\"Dnipro,UA\"><option value=\"Donetsk,UA\"><option value=\"Kyiv,UA\">");
  html += F("<option value=\"Uzhhorod,UA\"><option value=\"Zaporizhzhia,UA\"><option value=\"Ivano-Frankivsk,UA\"><option value=\"Kropyvnytskyi,UA\"><option value=\"Luhansk,UA\">");
  html += F("<option value=\"Lviv,UA\"><option value=\"Mykolaiv,UA\"><option value=\"Odesa,UA\"><option value=\"Poltava,UA\"><option value=\"Rivne,UA\"><option value=\"Sumy,UA\">");
  html += F("<option value=\"Ternopil,UA\"><option value=\"Kharkiv,UA\"><option value=\"Kherson,UA\"><option value=\"Khmelnytskyi,UA\"><option value=\"Cherkasy,UA\">");
  html += F("<option value=\"Chernivtsi,UA\"><option value=\"Chernihiv,UA\"><option value=\"Simferopol,UA\">");
  html += F("</datalist>");

  appendPortalInput(html, "BT device name", "btName", speakerConfig.btDeviceName);
  appendPortalInput(html, "Welcome screen text", "welcome", speakerConfig.welcomeText);
  appendPortalInput(html, "Portal user", "portalUser", speakerConfig.portalUser);
  appendPortalInput(html, "Portal password", "portalPass", speakerConfig.portalPass, "password");

  html += F("<button type=\"submit\">Save config</button>");
  html += F("<p class=\"hint\">After saving, reboot the speaker to apply Bluetooth name changes.</p>");
  html += F("</form></main></body></html>");

  return html;
}

bool configPortalAuthenticated() {
  normalizeSpeakerConfig();

  if (!configPortalServer.authenticate(speakerConfig.portalUser.c_str(), speakerConfig.portalPass.c_str())) {
    configPortalServer.requestAuthentication();
    return false;
  }

  return true;
}

void handleConfigPortalRoot() {
  if (!configPortalAuthenticated()) return;
  configPortalServer.send(200, "text/html; charset=utf-8", buildConfigPortalPage());
}

void handleConfigPortalSave() {
  if (!configPortalAuthenticated()) return;

  speakerConfig.wifiSsid = configPortalServer.arg("wifiSsid");
  speakerConfig.wifiPass = configPortalServer.arg("wifiPass");
  speakerConfig.weatherApiKey = configPortalServer.arg("weatherKey");
  speakerConfig.weatherLocation = configPortalServer.arg("weatherLoc");
  speakerConfig.btDeviceName = configPortalServer.arg("btName");
  speakerConfig.welcomeText = configPortalServer.arg("welcome");
  speakerConfig.portalUser = configPortalServer.arg("portalUser");
  speakerConfig.portalPass = configPortalServer.arg("portalPass");

  speakerConfig.wifiSsid.trim();
  speakerConfig.weatherApiKey.trim();
  speakerConfig.weatherLocation.trim();
  speakerConfig.btDeviceName.trim();
  speakerConfig.welcomeText.trim();
  speakerConfig.portalUser.trim();
  speakerConfig.portalPass.trim();

  bool saved = saveSpeakerConfig();
  configPortalServer.send(200, "text/html; charset=utf-8", buildConfigPortalPage(saved ? "Saved to NVS. Reboot the speaker to apply all changes." : "Save failed: NVS error."));

  Serial.println(saved ? "Config Portal: saved" : "Config Portal: save failed");
}

void handleConfigPortalNotFound() {
  configPortalServer.sendHeader("Location", "/", true);
  configPortalServer.send(302, "text/plain", "");
}

void drawConfigPortalScreen(const char* statusLine) {
  u8g2.clearBuffer();
  u8g2.drawFrame(5, 6, 118, 52);
  centerText("OLED-SETUP", 22, u8g2_font_6x10_tr);
  centerText("192.168.4.1", 36, u8g2_font_6x10_tr);
  centerText(statusLine, 50, u8g2_font_5x8_tr);
  u8g2.sendBuffer();
}

bool startConfigPortal(const char* reason) {
  if (configPortalActive) return true;

  Serial.print("Config Portal: starting AP, reason: ");
  Serial.println(reason);

  WiFi.disconnect(true);
  delay(150);
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAP(CONFIG_PORTAL_AP_SSID)) {
    Serial.println("Config Portal: AP start failed");
    drawConfigPortalScreen("AP start failed");
    return false;
  }

  configPortalServer.on("/", HTTP_GET, handleConfigPortalRoot);
  configPortalServer.on("/save", HTTP_POST, handleConfigPortalSave);
  configPortalServer.onNotFound(handleConfigPortalNotFound);
  configPortalServer.begin();
  configPortalActive = true;

  Serial.print("Config Portal AP SSID: ");
  Serial.println(CONFIG_PORTAL_AP_SSID);
  Serial.print("Config Portal URL: ");
  Serial.println(CONFIG_PORTAL_URL);
  Serial.print("Config Portal IP: ");
  Serial.println(WiFi.softAPIP());

  drawConfigPortalScreen("Open in browser");
  return true;
}

void handleConfigPortal() {
  if (!configPortalActive) return;
  configPortalServer.handleClient();
}
