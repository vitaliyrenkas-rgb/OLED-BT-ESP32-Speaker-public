// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= STATE =================
#include <WebServer.h>

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);
//BluetoothA2DPSink a2dp_sink;
Preferences prefs;
WebServer configPortalServer(80);
bool configPortalActive = false;


struct SpeakerConfig {
  String wifiSsid;
  String wifiPass;
  String weatherApiKey;
  String weatherLocation;
  String weatherCity;
  String weatherCountry;
  String btDeviceName;
  String welcomeText;
  String portalUser;
  String portalPass;
  bool loadedFromNvs;
};

SpeakerConfig speakerConfig;

enum ScreenMode {
  SCREEN_PLAYER,
  SCREEN_CLOCK,
  SCREEN_WEATHER,
  SCREEN_MESSAGE,
  SCREEN_GREETING,
  SCREEN_SLEEP
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
bool batteryCharging = false; // OLED: true when USB/VBUS is present and battery is not full
bool usbPowerPresent = false;   // OLED: GPIO33 USB/VBUS sense through 100k/100k divider
unsigned long batteryLastChargeRiseMs = 0;

int volumePotRaw = 0;
int volumePotVolume = 100;
int volumePotAppliedVolume = -1;
unsigned long lastVolumePotReadMs = 0;
bool volumePotReady = false;
bool volumeOverlayActive = false;
unsigned long volumeOverlayLastChangeMs = 0;
int volumeOverlayPercent = 100;

float weatherTemp = 23.0;
float weatherFeels = 24.0;
int weatherHumidity = -1; // OLED v3.5-004: relative humidity %, -1 = unknown
String weatherState = "SUN"; // SUN / CLOUD / RAIN / SNOW
bool weatherIsNight = false;  // OLED v3.5-001: OpenWeather icon suffix n/d
String weatherDesc = "Погода";

unsigned long lastDraw = 0;
unsigned long lastBattery = 0;
unsigned long lastMarquee = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long messageUntil = 0;
unsigned long greetingUntil = 0;
unsigned long lastHeapLog = 0;
unsigned long lastUserInteractionMs = 0; // FIX v3.0: auto-return to Player idle timer
unsigned long sleepScreenEnteredMs = 0;

bool forceRedraw = true;

int titleOffset = 0;
int artistOffset = 0;
