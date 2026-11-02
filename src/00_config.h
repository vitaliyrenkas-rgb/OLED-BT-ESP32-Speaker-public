// RT-003 v5.0 firmware configuration.

// ================= CONFIG =================
const char* WIFI_SSID = "Your_Wi-Fi_Spot";
const char* WIFI_PASS = "Your_Wi-Fi_Password";

const char* WEATHER_API_KEY = "Your Weather Key";
const char* WEATHER_CITY = "Your_City";
const char* WEATHER_COUNTRY = "Your_Country";

const char* TZ_INFO = "EET-2EEST,M3.5.0/3,M10.5.0/4";

const uint32_t CONFIG_VERSION = 1;

const char* BUILD_VERSION = "5.0-019";

// Startup networking is fail-open: every phase is bounded so Bluetooth still
// starts when the AP is reachable but the WAN, DNS, NTP or weather API is not.
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 12000UL;
const unsigned long NTP_SYNC_TIMEOUT_MS = 8000UL;
const int32_t WEATHER_HTTP_CONNECT_TIMEOUT_MS = 3500;
const uint16_t WEATHER_HTTP_READ_TIMEOUT_MS = 5000;

const unsigned long WEATHER_UPDATE_INTERVAL = 1UL * 60UL * 60UL * 1000UL;
const unsigned long SLEEP_SCREEN_IDLE_MS = 5UL * 60UL * 1000UL;
// const unsigned long SLEEP_SCREEN_IDLE_MS = 15UL * 1000UL; // for testing purpose
const unsigned long SLEEP_Z_ANIMATION_MS = 650UL;

#define RT003_DEBUG_ADKEY 1  // bench only: set to 0 after HU-055 button thresholds are calibrated
const unsigned long ADKEY_DEBUG_LOG_INTERVAL_MS = 250UL;

const int LOW_BATTERY_WARNING_PERCENT = 5;
const unsigned long LOW_BATTERY_WARNING_INTERVAL_MS = 5UL * 60UL * 1000UL;
const unsigned long LOW_BATTERY_WARNING_DURATION_MS = 15UL * 1000UL;
const uint8_t LOW_BATTERY_WARNING_MAX_COUNT = 4;
