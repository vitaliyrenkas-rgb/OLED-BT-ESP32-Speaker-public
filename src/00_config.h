// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= CONFIG =================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* WEATHER_API_KEY = "YOUR_OPENWEATHER_API_KEY";
const char* WEATHER_CITY = "Kyiv";
const char* WEATHER_COUNTRY = "UA";

const char* TZ_INFO = "EET-2EEST,M3.5.0/3,M10.5.0/4";

const uint32_t CONFIG_VERSION = 1;

const char* BUILD_VERSION = "v3.5-012"; // Sleep screen kitty

const unsigned long WEATHER_UPDATE_INTERVAL = 1UL * 60UL * 60UL * 1000UL;
const unsigned long SLEEP_SCREEN_IDLE_MS = 5UL * 60UL * 1000UL;
// const unsigned long SLEEP_SCREEN_IDLE_MS = 15UL * 1000UL; // for testing purpose
const unsigned long SLEEP_Z_ANIMATION_MS = 650UL;
