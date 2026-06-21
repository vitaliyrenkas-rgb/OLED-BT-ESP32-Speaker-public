// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= WIFI / TIME / WEATHER =================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);

  // Important for ESP32 Wi-Fi + Bluetooth coexistence.
  // Do NOT use WiFi.setSleep(false) with active Bluetooth A2DP.
  WiFi.setSleep(true);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

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

  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               String(WEATHER_CITY) + "," + String(WEATHER_COUNTRY) +
               "&appid=" + String(WEATHER_API_KEY) +
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
