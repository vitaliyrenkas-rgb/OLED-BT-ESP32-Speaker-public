#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Preferences.h>
#include "nvs_flash.h"
#include <U8g2lib.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
// #include "driver/i2s.h"
#include <math.h>

//  8) UI patch v2.8:
//     - Wi-Fi icon redrawn smaller and kept above topbar line.
//     - language selection screen simplified: En left, Укр. right.
//     - Welcome screen supports both languages and uses two centered lines.
//     - metadata playing time support added via ESP_AVRC_MD_ATTR_PLAYING_TIME.
//     - old vertical equalizer replaced with brick equalizer around duration.
//     - marquee speed increased to 120 ms.
//     - config reset button can reset language/config both at boot and during runtime.
//  9) UI/reset patch v2.9:
//     - config reset moved from GPIO32/D32 to GPIO32/D32.
//     - reset button remains INPUT_PULLUP: D19 -> button -> GND.
//     - debug prints human-readable pressed state: pressed=1, not pressed=0.
//     - Wi-Fi icon redrawn as 3 simple arcs; no Wi-Fi = arcs crossed by slash.
//     - Player redraw sped up for smoother timer/EQ.
//     - track timer displays elapsed only: 00:35.
//     - fallback timer resets on title callback/reconnect and freezes on disconnect.
//  10) Playback/NVS patch v2.10:
//      - real PCM tap via a2dp_sink.set_stream_reader() added.
//      - brick equalizer now follows real audio level, not fake millis animation.
//      - timer and EQ pause when playback stops/pauses or PCM goes silent.
//      - track timer displays elapsed only: 00:35.
//      - fallback timer resets on TITLE callback / track restart.
//      - config reset moved to GPIO32/D32 and hard-erases NVS via nvs_flash_erase().
//      - extra fallback config reset combo: BTN_CLOCK + BTN_WEATHER hold 3 sec.
//      - Wi-Fi icon moved upward and redrawn as simple 3 arcs + slash when sync failed.
//  11) UI polish v2.11:
//      - Wi-Fi icon redrawn in cleaner pixel-art style.
//      - Wi-Fi icon aligned vertically with Bluetooth icon.
//      - EQ upgraded from 2 to 3 brick columns per side.
//      - EQ sensitivity increased for more lively movement.
//      - config reset pin moved from GPIO32/D32 to GPIO32/D32.
//  12) Safe config reset v2.12:
//      - runtime hard reset disabled to stop automatic reset loop.
//      - config reset is boot-only and requires stable D32 LOW hold.
//      - language screen no longer blocks forever; defaults to UA after timeout.
//      - language label uses ASCII 'UA' instead of Cyrillic 'Укр.' to avoid dot/missing glyph.
//  13) Language/weather UI patch v2.13:
//      - external hard reset button removed from runtime flow.
//      - config/language reset now uses BTN_PLAYER + BTN_WEATHER hold 5 sec.
//      - after combo reset, language selection screen is shown without hard reboot.
//      - nav label 'Пог.' changed to 'Погода'.
//      - Weather 'Відч.' uses Cyrillic-capable font.
//      - weatherState label localized: СОНЦЕ / ХМАРИ / ДОЩ / СНІГ.
//  14) Stable merge v3.0:
//      - v2.13 accepted as stable base.
//      - Ukrainian CLOUD label changed from ХМАРИ to ХМАРНО.
//      - Bluetooth icon made smaller to avoid touching topbar line.
//      - brick equalizer sensitivity increased.
//      - normal screen switching blocked while BTN_PLAYER + BTN_WEATHER reset combo is held.
//      - UX: Bluetooth connect auto-switches to Player.
//      - UX: if music is playing and user stays on Clock/Weather, UI returns to Player after 1 min idle.
//  15) v3.2 startup jingle + cosmetics:
//      - startup hi-fi/car-head-unit style jingle added.
//      - jingle plays through MAX98357A before Bluetooth A2DP starts.
//      - Ukrainian clear-weather label changed from СОНЦЕ to ЯСНО.
//      - brick equalizer raised by 2 px to avoid Cyrillic metadata overlap.
// =====================================================
//  OLED BT SPEAKER — HARD / REAL DEVICE SKETCH v3.2 JINGLE + UI COSMETICS
//  Real ESP32 only. NOT for Wokwi.
//
//  FIXES / CHANGES IN v2.7:
//  1) UI + Wi-Fi icon:
//     - compact Wi-Fi icon added near Bluetooth icon in topbar.
//     - filled icon = last Wi-Fi/NTP/weather sync OK.
//     - outline icon = Wi-Fi sync not available / failed.
//  2) NTP/weather without BT conflict:
//     - Wi-Fi is used only during startup sync/weather update.
//     - Wi-Fi is fully turned OFF before A2DP Bluetooth starts.
//     - ensureWiFi() is NOT used in loop.
//     - periodic weather refresh is disabled during active Bluetooth.
//  3) Cyrillic:
//     - date/calendar uses Ukrainian short day/month names.
//     - metadata title/artist use Cyrillic-capable U8g2 fonts.
//  4) Weather UI:
//     - filled cloud icon kept.
//     - weather icon at y=16.
//     - weatherState label under icon kept.
//  5) Battery unavailable icon:
//     - no X.
//     - one diagonal slash top-right -> bottom-left.
//  6) Heap/redraw diagnostics:
//     - Serial heap diagnostics every 10 seconds.
//     - OLED full redraw throttled to reduce audio hiccups.
//  7) Config / language:
//     - Preferences/NVS with CONFIG_VERSION.
//     - first boot / config version change shows Select Language screen.
//     - BTN_PLAYER = En, BTN_WEATHER = Укр.
//     - separate config reset button: GPIO32 / D32.
//       IMPORTANT: classic ESP32 has no GPIO192.
// =====================================================

// ================= CONFIG =================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* WEATHER_API_KEY = "e0e1d7b80eb11ad0Xa2363e97917ae16";
const char* WEATHER_CITY = "Kyiv";
const char* WEATHER_COUNTRY = "UA";

const char* TZ_INFO = "EET-2EEST,M3.5.0/3,M10.5.0/4";

const uint32_t CONFIG_VERSION = 1;
const unsigned long WEATHER_UPDATE_INTERVAL = 4UL * 60UL * 60UL * 1000UL;

// ================= OLED / REAL DEVICE PINS =================
#define OLED_SDA 23
#define OLED_SCL 22

// Your test module behaved as SH1106 in previous hard sketches.
// If the real OLED is SSD1306 and image is shifted/wrong, replace constructor with:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ================= MAX98357A / I2S =================
// OLED uses GPIO22 for SCL, so I2S DOUT is moved to GPIO32.
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 32



// ================= BUTTONS =================
// INPUT_PULLUP: button connects GPIO to GND when pressed.
#define BTN_PLAYER   13
#define BTN_CLOCK    16
#define BTN_WEATHER  17

// Separate service/config reset button, hold 2.5s during boot.
// Do NOT use GPIO192 on classic ESP32: it does not exist.
#define CONFIG_RESET_PIN 35

// ================= BATTERY ADC =================
#define BATTERY_ADC_PIN 34
const float ADC_REF_VOLTAGE = 3.30;
const float ADC_MAX = 4095.0;
const float DIVIDER_RATIO = 2.0;
const float BATTERY_ABSENT_VOLTAGE = 0.50;

// ================= STATE =================

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);
//BluetoothA2DPSink a2dp_sink;
Preferences prefs;

enum ScreenMode {
  SCREEN_PLAYER,
  SCREEN_CLOCK,
  SCREEN_WEATHER,
  SCREEN_MESSAGE,
  SCREEN_GREETING
};

void drawNavBar(ScreenMode active);


enum UiLanguage {
  LANG_EN,
  LANG_UA
};

ScreenMode currentScreen = SCREEN_GREETING;
ScreenMode returnScreen = SCREEN_CLOCK;
UiLanguage uiLang = LANG_UA;

bool btConnected = false;
bool manualScreenLock = false;
bool btAutoSwitchedToPlayer = false;
bool ntpConfigured = false;
bool ntpSynced = false;
bool wifiLastSyncOk = false;

String title = "Очікування...";
String artist = "Підключи телефон";
String album = "";

// FIX v2.10: real PCM/playback state for timer and EQ.
volatile uint16_t pcmLevelRaw = 0;
volatile unsigned long lastPcmAudioMs = 0;
bool trackTimerRunning = false;
bool playbackActive = false;
unsigned long lastPlaybackStateChangeMs = 0;

// FIX v2.8:
// AVRCP can provide total playing time as ESP_AVRC_MD_ATTR_PLAYING_TIME.
// Some phones/apps do not send it, so we keep a local elapsed fallback.
uint32_t trackDurationMs = 0;
uint32_t trackStartedAtMs = 0;
uint32_t trackElapsedOffsetMs = 0;

float batteryVoltage = 0.0;
int batteryPercent = 85;
bool batteryPresent = true;

float weatherTemp = 23.0;
float weatherFeels = 24.0;
String weatherState = "SUN"; // SUN / CLOUD / RAIN / SNOW
String weatherDesc = "Погода";

unsigned long lastDraw = 0;
unsigned long lastBattery = 0;
unsigned long lastMarquee = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long messageUntil = 0;
unsigned long greetingUntil = 0;
unsigned long lastHeapLog = 0;
unsigned long lastUserInteractionMs = 0; // FIX v3.0: auto-return to Player idle timer

bool forceRedraw = true;

int titleOffset = 0;
int artistOffset = 0;

// ================= TEXT HELPERS =================
int textW(const String &s) {
  return u8g2.getUTF8Width(s.c_str());
}

void requestRedraw() {
  forceRedraw = true;
}

void centerTextSafe(const String &s, int y, const uint8_t *font) {
  u8g2.setFont(font);
  int w = u8g2.getUTF8Width(s.c_str());
  int x = (128 - w) / 2;
  if (x < 0) x = 0;
  u8g2.setCursor(x, y);
  u8g2.print(s);
}

void centerText(const String &s, int y, const uint8_t *font) {
  centerTextSafe(s, y, font);
}

bool isTimeValid() {
  struct tm t;
  if (!getLocalTime(&t, 5)) return false;
  return t.tm_year >= (2024 - 1900);
}

String timeStr() {
  struct tm t;
  if (!getLocalTime(&t, 5) || t.tm_year < (2024 - 1900)) return "--:--";

  char buf[6];
  sprintf(buf, "%02d:%02d", t.tm_hour, t.tm_min);
  return String(buf);
}

String dateStr() {
  struct tm t;
  if (!getLocalTime(&t, 5) || t.tm_year < (2024 - 1900)) {
    return uiLang == LANG_UA ? "час не синхр." : "time not sync";
  }

  if (uiLang == LANG_UA) {
    const char* days[] = { "Нд", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб" };
    const char* months[] = {
      "січ.", "лют.", "бер.", "квіт.", "трав.", "черв.",
      "лип.", "серп.", "вер.", "жовт.", "лист.", "груд."
    };
    return String(days[t.tm_wday]) + ", " + String(t.tm_mday) + " " + String(months[t.tm_mon]);
  } else {
    const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    const char* months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return String(days[t.tm_wday]) + ", " + String(t.tm_mday) + " " + String(months[t.tm_mon]);
  }
}

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

// ================= BATTERY =================
float readBatteryVoltage() {
  long sum = 0;

  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
  }

  float raw = sum / 8.0;
  float adcVoltage = (raw / ADC_MAX) * ADC_REF_VOLTAGE;
  return adcVoltage * DIVIDER_RATIO;
}

int voltageToPercent(float v) {
  if (v < BATTERY_ABSENT_VOLTAGE) return 0;
  if (v >= 4.20) return 100;
  if (v >= 4.10) return 90;
  if (v >= 4.00) return 80;
  if (v >= 3.90) return 70;
  if (v >= 3.80) return 60;
  if (v >= 3.70) return 50;
  if (v >= 3.60) return 35;
  if (v >= 3.50) return 20;
  if (v >= 3.40) return 10;
  if (v >= 3.30) return 5;
  return 0;
}

void updateBattery() {
  batteryVoltage = readBatteryVoltage();
  batteryPresent = batteryVoltage >= BATTERY_ABSENT_VOLTAGE;
  batteryPercent = voltageToPercent(batteryVoltage);
}

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
      int weatherId = doc["weather"][0]["id"] | 800;
      const char* desc = doc["weather"][0]["description"] | "погода";
      weatherDesc = String(desc);

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

// ================= ICONS =================
void drawBTIconSmall(int x, int y) {
  // FIX v3.0:
  // Smaller Bluetooth icon so it does not touch the topbar line.
  if (!btConnected) {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(x, y + 7, "BT");
    return;
  }

  // Compact 7px high BT rune.
  u8g2.drawLine(x + 3, y + 1, x + 3, y + 8);
  u8g2.drawLine(x + 3, y + 1, x + 7, y + 3);
  u8g2.drawLine(x + 7, y + 3, x + 3, y + 5);
  u8g2.drawLine(x + 3, y + 5, x + 7, y + 7);
  u8g2.drawLine(x + 7, y + 7, x + 3, y + 8);
  u8g2.drawLine(x + 1, y + 3, x + 3, y + 5);
  u8g2.drawLine(x + 1, y + 7, x + 3, y + 5);
}

void drawWiFiIcon(int x, int y, bool connected) {
  // FIX v2.11:
  // Cleaner pixel-art Wi-Fi icon aligned with BT icon.

  // outer arc
  u8g2.drawPixel(x + 0, y + 5);
  u8g2.drawPixel(x + 1, y + 4);
  u8g2.drawPixel(x + 2, y + 3);
  u8g2.drawPixel(x + 6, y + 3);
  u8g2.drawPixel(x + 7, y + 4);
  u8g2.drawPixel(x + 8, y + 5);

  // middle arc
  u8g2.drawPixel(x + 2, y + 6);
  u8g2.drawPixel(x + 3, y + 5);
  u8g2.drawPixel(x + 5, y + 5);
  u8g2.drawPixel(x + 6, y + 6);

  // inner arc
  u8g2.drawPixel(x + 3, y + 7);
  u8g2.drawPixel(x + 4, y + 6);
  u8g2.drawPixel(x + 5, y + 7);

  // center dot
  u8g2.drawPixel(x + 4, y + 9);

  if (!connected) {
    u8g2.drawLine(x + 9, y + 0, x + 0, y + 10);
  }
}

void drawBatteryUnavailableSlash(int x, int y) {
  // One diagonal slash, no X.
  // Final reference: top-right -> bottom-left.
  u8g2.drawLine(x + 20, y - 1, x + 1, y + 9);
}

void drawBatteryIconCompact(int x, int y) {
  u8g2.drawFrame(x, y, 20, 8);
  u8g2.drawBox(x + 20, y + 2, 2, 4);

  if (!batteryPresent || batteryPercent <= 0) {
    drawBatteryUnavailableSlash(x, y);
    return;
  }

  int fill = map(batteryPercent, 0, 100, 0, 18);
  fill = constrain(fill, 0, 18);
  u8g2.drawBox(x + 1, y + 1, fill, 6);
}

void drawSunIcon(int x, int y) {
  u8g2.drawDisc(x + 8, y + 8, 4);
  u8g2.drawLine(x + 8, y + 1, x + 8, y + 3);
  u8g2.drawLine(x + 8, y + 13, x + 8, y + 15);
  u8g2.drawLine(x + 1, y + 8, x + 3, y + 8);
  u8g2.drawLine(x + 13, y + 8, x + 15, y + 8);
  u8g2.drawLine(x + 3, y + 3, x + 5, y + 5);
  u8g2.drawLine(x + 13, y + 3, x + 11, y + 5);
  u8g2.drawLine(x + 3, y + 13, x + 5, y + 11);
  u8g2.drawLine(x + 13, y + 13, x + 11, y + 11);
}

void drawCloudIcon(int x, int y) {
  // Filled cloud icon, not outline.
  u8g2.drawDisc(x + 6,  y + 10, 4);
  u8g2.drawDisc(x + 12, y + 8,  5);
  u8g2.drawDisc(x + 18, y + 11, 4);
  u8g2.drawBox(x + 4, y + 10, 17, 5);
  u8g2.drawHLine(x + 4, y + 15, 17);
}

void drawRainIcon(int x, int y) {
  drawCloudIcon(x, y);
  u8g2.drawLine(x + 6, y + 17, x + 4, y + 21);
  u8g2.drawLine(x + 12, y + 17, x + 10, y + 21);
  u8g2.drawLine(x + 18, y + 17, x + 16, y + 21);
}

void drawSnowIcon(int x, int y) {
  drawCloudIcon(x, y);
  u8g2.drawPixel(x + 6, y + 18);
  u8g2.drawPixel(x + 12, y + 20);
  u8g2.drawPixel(x + 18, y + 18);
}

void drawWeatherIcon(int x, int y) {
  if (weatherState == "SUN") drawSunIcon(x, y);
  else if (weatherState == "RAIN") drawRainIcon(x, y);
  else if (weatherState == "SNOW") drawSnowIcon(x, y);
  else drawCloudIcon(x, y);
}

String weatherStateLabel() {
  // FIX v2.13:
  // Localized short weather state label under icon.
  if (uiLang == LANG_UA) {
    if (weatherState == "SUN") return "ЯСНО";
    if (weatherState == "CLOUD") return "ХМАРНО";
    if (weatherState == "RAIN") return "ДОЩ";
    if (weatherState == "SNOW") return "СНІГ";
    return "ПОГ.";
  }

  return weatherState;
}

// ================= COMMON UI =================
void drawTopBar() {
  drawBTIconSmall(2, 1);
  drawWiFiIcon(17, -1, wifiLastSyncOk); // FIX v2.8: smaller icon now fits above topbar line

  u8g2.setFont(u8g2_font_6x10_tr);
  String topTime = timeStr();
  int timeW = u8g2.getUTF8Width(topTime.c_str());
  u8g2.setCursor((128 - timeW) / 2 - 4, 9);
  u8g2.print(topTime);

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(80, 9);
  u8g2.print(String((int)weatherTemp));
  u8g2.print("C");

  drawBatteryIconCompact(104, 2);
  u8g2.drawHLine(0, 12, 128);
}

void drawNavItem(int x, int w, const char* label, bool active) {
  if (active) {
    u8g2.drawBox(x, 55, w, 9);
    u8g2.setDrawColor(0);
  }

  u8g2.setFont(uiLang == LANG_UA ? u8g2_font_6x12_t_cyrillic : u8g2_font_5x8_tr);
  int tw = u8g2.getUTF8Width(label);
  int tx = x + (w - tw) / 2;
  if (tx < x) tx = x;
  u8g2.setCursor(tx, 63);
  u8g2.print(label);

  if (active) u8g2.setDrawColor(1);
}

void drawNavBar(ScreenMode active) {
  u8g2.drawHLine(0, 54, 128);

  if (uiLang == LANG_UA) {
    drawNavItem(0, 42, "Плеєр", active == SCREEN_PLAYER);
    drawNavItem(43, 39, "ЧАС", active == SCREEN_CLOCK);
    drawNavItem(83, 45, "Погода", active == SCREEN_WEATHER);
  } else {
    drawNavItem(0, 42, "Player", active == SCREEN_PLAYER);
    drawNavItem(43, 39, "Clock", active == SCREEN_CLOCK);
    drawNavItem(83, 45, "Weather", active == SCREEN_WEATHER);
  }
}

// ================= PLAYBACK / PCM HELPERS =================
void startTrackTimer() {
  if (!trackTimerRunning) {
    trackStartedAtMs = millis();
    trackTimerRunning = true;
    lastPlaybackStateChangeMs = millis();
  }
}

void pauseTrackTimer() {
  if (trackTimerRunning) {
    trackElapsedOffsetMs += millis() - trackStartedAtMs;
    trackTimerRunning = false;
    lastPlaybackStateChangeMs = millis();
  }
}

void resetTrackTimer() {
  trackElapsedOffsetMs = 0;
  trackStartedAtMs = millis();
  trackTimerRunning = playbackActive;
}

void setPlaybackActive(bool active) {
  if (playbackActive == active) return;

  playbackActive = active;

  if (playbackActive) startTrackTimer();
  else pauseTrackTimer();

  requestRedraw();
}

uint8_t audioLevelToBricks() {
  uint16_t level = pcmLevelRaw;

  if (!playbackActive || millis() - lastPcmAudioMs > 700) return 0;

  // FIX v2.11:
  // Increased sensitivity for more lively EQ movement.
  // FIX v3.0:
  // Slightly more sensitive EQ response.
  if (level < 120) return 0;
  if (level < 350) return 1;
  if (level < 800) return 2;
  if (level < 1600) return 3;
  if (level < 3200) return 4;
  if (level < 6000) return 5;
  return 6;
}

// PCM tap from ESP32-A2DP.
// API: a2dp_sink.set_stream_reader(read_data_stream);
// Data is normally 44.1kHz, stereo, 16-bit PCM.
void read_data_stream(const uint8_t *data, uint32_t length) {
  const int16_t *samples = (const int16_t*)data;
  uint32_t sampleCount = length / 2;

  uint32_t sumAbs = 0;
  uint32_t used = 0;

  // Keep callback light: sample only every 8th int16.
  for (uint32_t i = 0; i < sampleCount; i += 8) {
    int32_t s = samples[i];
    if (s < 0) s = -s;
    sumAbs += (uint32_t)s;
    used++;
  }

  if (used == 0) return;

  uint16_t avg = (uint16_t)(sumAbs / used);
  pcmLevelRaw = avg;

  if (avg > 250) lastPcmAudioMs = millis();
}

void playback_status_callback(esp_avrc_playback_stat_t playback) {
  // FIX v2.10: pause/stop freezes both timer and EQ.
  if (playback == ESP_AVRC_PLAYBACK_PLAYING) setPlaybackActive(true);
  else setPlaybackActive(false);
}

// ================= TRACK TIME HELPERS =================
String formatTrackTime(uint32_t ms) {
  uint32_t totalSec = ms / 1000;
  uint16_t minutes = totalSec / 60;
  uint8_t seconds = totalSec % 60;

  char buf[8];
  sprintf(buf, "%02u:%02u", minutes, seconds);
  return String(buf);
}

uint32_t currentTrackPositionMs() {
  if (!btConnected) return 0;
  if (trackTimerRunning) return trackElapsedOffsetMs + (millis() - trackStartedAtMs);
  return trackElapsedOffsetMs;
}

String durationDisplayStr() {
  // FIX v2.10: elapsed only, no total duration.
  return formatTrackTime(currentTrackPositionMs());
}

// ================= PLAYER HELPERS =================
void drawMarqueeLine(const String &s, int y, int &offset, const uint8_t *font, int left, int right) {
  u8g2.setFont(font);

  int maxW = right - left;
  int w = u8g2.getUTF8Width(s.c_str());

  if (w <= maxW) {
    int x = left + (maxW - w) / 2;
    u8g2.setCursor(x, y);
    u8g2.print(s);
    return;
  }

  if (millis() - lastMarquee > 120) {
    offset--;
    lastMarquee = millis();
    requestRedraw();
  }

  if (offset < -w - 18) offset = maxW;

  u8g2.setCursor(left + offset, y);
  u8g2.print(s);

  u8g2.setCursor(left + offset + w + 18, y);
  u8g2.print(s);
}

void drawBrickEqColumn(int x, int baseY, int bricks, int maxBricks) {
  // FIX v2.8:
  // Brick equalizer, accepted as the new UI direction.
  const int brickW = 3;
  const int brickH = 2;
  const int gap = 1;

  bricks = constrain(bricks, 0, maxBricks);

  for (int i = 0; i < bricks; i++) {
    int y = baseY - i * (brickH + gap);
    u8g2.drawBox(x, y, brickW, brickH);
  }
}

void drawBrickEqualizer(int leftX, int rightX, int baseY) {
  const int maxBricks = 6;

  // FIX v2.11:
  // 3-column EQ per side, driven by real PCM level.
  uint8_t level = audioLevelToBricks();

  if (!playbackActive) level = 0;

  int l1 = level;
  int l2 = level > 1 ? level - 1 : level;
  int l3 = level > 2 ? level - 2 : level;

  int r1 = level > 2 ? level - 2 : level;
  int r2 = level > 1 ? level - 1 : level;
  int r3 = level;

  drawBrickEqColumn(leftX,       baseY, l1, maxBricks);
  drawBrickEqColumn(leftX + 5,   baseY, l2, maxBricks);
  drawBrickEqColumn(leftX + 10,  baseY, l3, maxBricks);

  drawBrickEqColumn(rightX,      baseY, r1, maxBricks);
  drawBrickEqColumn(rightX + 5,  baseY, r2, maxBricks);
  drawBrickEqColumn(rightX + 10, baseY, r3, maxBricks);
}

// ================= SCREENS =================
void drawGreetingScreen() {
  u8g2.drawFrame(8, 12, 112, 40);

  // FIX v2.8:
  // Welcome screen supports both languages and uses two centered lines,
  // so text does not go outside the frame.
  if (uiLang == LANG_UA) {
    centerText("Привіт,", 29, u8g2_font_6x12_t_cyrillic);
    centerText("OLED BT Speaker", 44, u8g2_font_6x12_t_cyrillic);
  } else {
    centerText("Welcome", 29, u8g2_font_6x13_tr);
    centerText("OLED BT Speaker", 44, u8g2_font_6x13_tr);
  }

  u8g2.drawHLine(24, 49, 80);
}

void drawClockScreen() {
  drawTopBar();

  centerText(timeStr(), 40, u8g2_font_logisoso26_tn);
  centerText(dateStr(), 52, uiLang == LANG_UA ? u8g2_font_6x12_t_cyrillic : u8g2_font_6x10_tr);

  drawNavBar(SCREEN_CLOCK);
}

void drawPlayerScreen() {
  drawTopBar();

  if (!btConnected) {
    if (uiLang == LANG_UA) {
      centerText("BT не підключено", 34, u8g2_font_6x12_t_cyrillic);
      centerText("Час / Погода OK", 47, u8g2_font_5x8_tr);
    } else {
      centerText("BT not connected", 34, u8g2_font_6x10_tr);
      centerText("Clock / Weather OK", 47, u8g2_font_5x8_tr);
    }

    drawNavBar(SCREEN_PLAYER);
    return;
  }

  // FIX v2.8:
  // Dynamic duration from AVRCP PLAYING_TIME + local elapsed fallback.
  String dur = durationDisplayStr();

  u8g2.setFont(u8g2_font_logisoso18_tn);
  int durW = u8g2.getUTF8Width(dur.c_str());
  int durX = (128 - durW) / 2;
  if (durX < 30) durX = 30;
  u8g2.setCursor(durX, 34);
  u8g2.print(dur);

  // FIX v2.8:
  // New brick equalizer around duration.
  drawBrickEqualizer(durX - 20, durX + durW + 3, 35);

  // Cyrillic-capable metadata lines.
  drawMarqueeLine(title, 45, titleOffset, u8g2_font_6x12_t_cyrillic, 4, 124);
  drawMarqueeLine(artist, 53, artistOffset, u8g2_font_4x6_t_cyrillic, 4, 124);

  drawNavBar(SCREEN_PLAYER);
}

void drawWeatherScreen() {
  drawTopBar();

  drawWeatherIcon(9, 16);

  u8g2.setFont(u8g2_font_logisoso18_tn);
  u8g2.setCursor(50, 39);
  u8g2.print(String((int)weatherTemp));

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(80, 27, "C");

  // FIX v2.13:
  // Ukrainian "Відч." needs Cyrillic-capable font.
  if (uiLang == LANG_UA) {
    u8g2.setFont(u8g2_font_4x6_t_cyrillic);
    u8g2.setCursor(48, 50);
    u8g2.print("Відч.");
  } else {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(48, 50);
    u8g2.print("Feels ");
  }

  u8g2.print(String((int)weatherFeels));
  u8g2.print("C");

  // FIX v2.13:
  // Weather state label localized.
  u8g2.setFont(uiLang == LANG_UA ? u8g2_font_4x6_t_cyrillic : u8g2_font_5x8_tr);
  u8g2.setCursor(9, 50);
  u8g2.print(weatherStateLabel());

  drawNavBar(SCREEN_WEATHER);
}

void drawMessageScreen() {
  u8g2.drawFrame(8, 12, 112, 40);

  if (uiLang == LANG_UA) {
    centerText("Bluetooth", 28, u8g2_font_6x13_tr);
    centerText("не підключено", 42, u8g2_font_6x12_t_cyrillic);
  } else {
    centerText("Bluetooth", 28, u8g2_font_6x13_tr);
    centerText("is not connected", 42, u8g2_font_6x10_tr);
  }

  u8g2.drawHLine(24, 49, 80);
}

void drawUI() {
  u8g2.clearBuffer();

  if (currentScreen == SCREEN_PLAYER) drawPlayerScreen();
  else if (currentScreen == SCREEN_WEATHER) drawWeatherScreen();
  else if (currentScreen == SCREEN_MESSAGE) drawMessageScreen();
  else if (currentScreen == SCREEN_GREETING) drawGreetingScreen();
  else drawClockScreen();

  u8g2.sendBuffer();
}

bool resetComboHeld() {
  return digitalRead(BTN_PLAYER) == LOW && digitalRead(BTN_WEATHER) == LOW;
}

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

    if (btConnected) {
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

// ================= CALLBACKS =================
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  String value = String((const char*)text);

  if (id == ESP_AVRC_MD_ATTR_TITLE) {
    // FIX v2.10:
    // Reset fallback timer whenever TITLE is received again.
    // Covers next track and often repeat-one restart.
    title = value;
    titleOffset = 0;
    resetTrackTimer();
    requestRedraw();
  }

  if (id == ESP_AVRC_MD_ATTR_ARTIST) {
    artist = value;
    artistOffset = 0;
    requestRedraw();
  }

  if (id == ESP_AVRC_MD_ATTR_ALBUM) {
    album = value;
  }

  if (id == ESP_AVRC_MD_ATTR_PLAYING_TIME) {
    // FIX v2.8:
    // AVRCP playing time is usually provided as a string in milliseconds.
    trackDurationMs = (uint32_t)value.toInt();
    trackStartedAtMs = millis();
    trackElapsedOffsetMs = 0;
    requestRedraw();
  }
}

void avrc_connection_state_callback(bool connected) {
  btConnected = connected;

  if (connected) {
    lastUserInteractionMs = millis();
    trackStartedAtMs = millis();
    trackElapsedOffsetMs = 0;

    if (!btAutoSwitchedToPlayer && !manualScreenLock && currentScreen != SCREEN_GREETING) {
      currentScreen = SCREEN_PLAYER;
      btAutoSwitchedToPlayer = true;
    }
  } else {
    title = "Очікування...";
    artist = "Підключи телефон";
    album = "";
    trackDurationMs = 0;
    trackStartedAtMs = 0;
    trackElapsedOffsetMs = 0;
    playbackActive = false;
    trackTimerRunning = false;
    pcmLevelRaw = 0;
    titleOffset = 0;
    artistOffset = 0;
    btAutoSwitchedToPlayer = false;
    manualScreenLock = false;

    if (currentScreen != SCREEN_GREETING) currentScreen = SCREEN_CLOCK;
  }

  requestRedraw();
}

// ================= DIAGNOSTICS =================
void logHeapDiagnostics() {
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" | Min free heap: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.print(" | Screen: ");
  Serial.print(currentScreen);
  Serial.print(" | BT: ");
  Serial.print(btConnected ? "ON" : "OFF");
  Serial.print(" | WiFiSync: ");
  Serial.println(wifiLastSyncOk ? "OK" : "FAIL");
}

void hardResetConfigAndRestart() {
  Serial.println("CONFIG RESET: hard erase NVS");
  prefs.clear();
  prefs.end();

  // Uploading firmware does NOT erase Preferences/NVS.
  // This hard-erases NVS to force language/config selection again.
  nvs_flash_erase();
  nvs_flash_init();

  delay(800);
  ESP.restart();
}

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

// ================= STARTUP JINGLE =================
// void playJingleTone(float freq, int durationMs, float volume = 0.22f) {
//   const int sampleRate = 44100;
//   const int samples = (sampleRate * durationMs) / 1000;
//   const int chunk = 128;

//   int16_t buffer[chunk * 2]; // stereo: L/R
//   size_t bytesWritten = 0;

//   for (int i = 0; i < samples; i += chunk) {
//     int count = min(chunk, samples - i);

//     for (int j = 0; j < count; j++) {
//       int idx = i + j;
//       float t = (float)idx / sampleRate;

//       // Small fade in/out to avoid clicks.
//       float fade = 1.0f;
//       int fadeSamples = min(300, samples / 8);
//       if (idx < fadeSamples) fade = (float)idx / fadeSamples;
//       else if (idx > samples - fadeSamples) fade = (float)(samples - idx) / fadeSamples;

//       int16_t sample = (int16_t)(sin(2.0f * PI * freq * t) * 32767.0f * volume * fade);

//       buffer[j * 2 + 0] = sample;
//       buffer[j * 2 + 1] = sample;
//     }

//     i2s_write(I2S_NUM_0, buffer, count * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
//   }
// }

// void playJingleSilence(int durationMs) {
//   const int sampleRate = 44100;
//   const int samples = (sampleRate * durationMs) / 1000;
//   const int chunk = 128;

//   int16_t buffer[chunk * 2] = {0};
//   size_t bytesWritten = 0;

//   for (int i = 0; i < samples; i += chunk) {
//     int count = min(chunk, samples - i);
//     i2s_write(I2S_NUM_0, buffer, count * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
//   }
// }

// void playStartupJingle() {
//   // FIX v3.2:
//   // Short hi-fi / early-MP3-car-head-unit style startup sound.
//   // Plays before Bluetooth A2DP starts, so it does not fight the BT audio stream.

//   i2s_config_t jingle_i2s_config = {
//     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
//     .sample_rate = 44100,
//     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
//     .communication_format = I2S_COMM_FORMAT_STAND_I2S,
//     .intr_alloc_flags = 0,
//     .dma_buf_count = 8,
//     .dma_buf_len = 64,
//     .use_apll = false,
//     .tx_desc_auto_clear = true,
//     .fixed_mclk = 0
//   };

//   i2s_pin_config_t jingle_pin_config = {
//     .bck_io_num = I2S_BCLK,
//     .ws_io_num = I2S_LRC,
//     .data_out_num = I2S_DOUT,
//     .data_in_num = I2S_PIN_NO_CHANGE
//   };

//   i2s_driver_install(I2S_NUM_0, &jingle_i2s_config, 0, NULL);
//   i2s_set_pin(I2S_NUM_0, &jingle_pin_config);
//   i2s_zero_dma_buffer(I2S_NUM_0);

//   // Selected "hifi" jingle: C5 -> E5 -> A5.
//   playJingleTone(523.25f, 100);
//   playJingleSilence(20);
//   playJingleTone(659.25f, 100);
//   playJingleSilence(20);
//   playJingleTone(880.00f, 240);

//   playJingleSilence(80);
//   i2s_driver_uninstall(I2S_NUM_0);
// }


// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  updateBattery();

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  u8g2.enableUTF8Print();

  setupButtons();
  // FIX v2.13: No external reset pin. Config reset is BTN_PLAYER + BTN_WEATHER hold 5s.
  Serial.println("Config reset: hold BTN_PLAYER + BTN_WEATHER for 5 seconds");
  loadOrSelectLanguage();

  currentScreen = SCREEN_GREETING;
  greetingUntil = millis() + 2500;
  lastUserInteractionMs = millis();
  drawUI();

  // Startup sync: Wi-Fi -> NTP/weather -> Wi-Fi OFF.
  // Bluetooth starts only after Wi-Fi is off.
  updateWeatherCycle();

  // FIX v3.2: play short startup jingle before A2DP owns I2S.
  // playStartupJingle();

auto cfg = i2s.defaultConfig(TX_MODE);
cfg.pin_bck = I2S_BCLK;
cfg.pin_ws = I2S_LRC;
cfg.pin_data = I2S_DOUT;
i2s.begin(cfg);

  a2dp_sink.set_avrc_connection_state_callback(avrc_connection_state_callback);
  a2dp_sink.set_avrc_metadata_attribute_mask(
    ESP_AVRC_MD_ATTR_TITLE |
    ESP_AVRC_MD_ATTR_ARTIST |
    ESP_AVRC_MD_ATTR_ALBUM |
    ESP_AVRC_MD_ATTR_PLAYING_TIME
  );
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  // FIX v2.10: real audio level from PCM stream + AVRCP playback status.
  a2dp_sink.set_stream_reader(read_data_stream);
  a2dp_sink.set_avrc_rn_playstatus_callback(playback_status_callback);

  a2dp_sink.start("OLED BT Speaker");

  requestRedraw();
}

// ================= LOOP =================
void loop() {
  unsigned long now = millis();

  if (currentScreen == SCREEN_GREETING && now >= greetingUntil) {
    currentScreen = btConnected ? SCREEN_PLAYER : SCREEN_CLOCK;
    requestRedraw();
  }

  // No background Wi-Fi here.
  // It caused A2DP stutter in previous builds.
  handleConfigResetButton();
  handleButtons();

  // FIX v3.0:
  // If Bluetooth is connected and music is playing, but user left UI on Clock/Weather,
  // return to Player after 1 minute of no button activity.
  if (btConnected && playbackActive &&
      (currentScreen == SCREEN_CLOCK || currentScreen == SCREEN_WEATHER) &&
      millis() - lastUserInteractionMs > 60000) {
    currentScreen = SCREEN_PLAYER;
    manualScreenLock = false;
    lastUserInteractionMs = millis();
    requestRedraw();
  }



  // FIX v2.10:
  // If AVRCP playstatus is not delivered by phone/browser, infer pause/stop from PCM silence.
  if (btConnected && playbackActive && millis() - lastPcmAudioMs > 1200) {
    setPlaybackActive(false);
  }

  // If PCM resumes without AVRCP status, resume timer/EQ.
  if (btConnected && !playbackActive && millis() - lastPcmAudioMs < 500 && pcmLevelRaw > 250) {
    setPlaybackActive(true);
  }



  if (now - lastBattery > 10000) {
    lastBattery = now;
    updateBattery();
    requestRedraw();
  }

  // Periodic weather refresh intentionally disabled during BT era.
  // Later: move refresh into advanced menu.
  // if (!btConnected && now - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
  //   updateWeatherCycle();
  // }

  if (now - lastHeapLog > 10000) {
    lastHeapLog = now;
    logHeapDiagnostics();
  }

  // Redraw throttle:
  // - Player with BT: slower, to reduce I2C/OLED pressure during A2DP.
  // - forced redraw still happens immediately after button/metadata changes.
  unsigned long refreshRate = 1000;
  if (btConnected && currentScreen == SCREEN_PLAYER) refreshRate = 120; // FIX v2.9: smooth timer/EQ
  if (currentScreen == SCREEN_CLOCK) refreshRate = 1000;
  if (currentScreen == SCREEN_WEATHER) refreshRate = 1000;

  if (forceRedraw || now - lastDraw > refreshRate) {
    forceRedraw = false;
    lastDraw = now;
    drawUI();
  }
}
