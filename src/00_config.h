// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= CONFIG =================
const char* WIFI_SSID = "Your_Wi-Fi_Spot";
const char* WIFI_PASS = "Your_Wi-Fi_Password";

const char* WEATHER_API_KEY = "Your Weather Key";
const char* WEATHER_CITY = "Your_City";
const char* WEATHER_COUNTRY = "Your_Country";

const char* TZ_INFO = "EET-2EEST,M3.5.0/3,M10.5.0/4";

const uint32_t CONFIG_VERSION = 1;

const char* BUILD_VERSION = "4.0-008";

const unsigned long WEATHER_UPDATE_INTERVAL = 1UL * 60UL * 60UL * 1000UL;
const unsigned long SLEEP_SCREEN_IDLE_MS = 5UL * 60UL * 1000UL;
// const unsigned long SLEEP_SCREEN_IDLE_MS = 15UL * 1000UL; // for testing purpose
const unsigned long SLEEP_Z_ANIMATION_MS = 650UL;

#define OLEG4_DEBUG_ADKEY 0  // bench only: set to 0 after HU-055 button thresholds are calibrated
const unsigned long ADKEY_DEBUG_LOG_INTERVAL_MS = 250UL;

const int LOW_BATTERY_WARNING_PERCENT = 5;
const unsigned long LOW_BATTERY_WARNING_INTERVAL_MS = 5UL * 60UL * 1000UL;
const unsigned long LOW_BATTERY_WARNING_DURATION_MS = 15UL * 1000UL;
const uint8_t LOW_BATTERY_WARNING_MAX_COUNT = 4;
