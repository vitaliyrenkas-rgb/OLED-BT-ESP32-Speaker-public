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

const char* BUILD_VERSION = "v3.5-009"; // AP Config Portal 

const unsigned long WEATHER_UPDATE_INTERVAL = 1UL * 60UL * 60UL * 1000UL;
