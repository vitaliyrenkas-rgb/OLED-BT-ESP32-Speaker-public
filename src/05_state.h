// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

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
