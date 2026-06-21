// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= CONFIG =================
const char* WIFI_SSID = "TP-Link_2A04";
const char* WIFI_PASS = "62902366";

const char* WEATHER_API_KEY = "e0e1d7b80eb11ad0da2363e97917ae16";
const char* WEATHER_CITY = "Kyiv";
const char* WEATHER_COUNTRY = "UA";

const char* TZ_INFO = "EET-2EEST,M3.5.0/3,M10.5.0/4";

const uint32_t CONFIG_VERSION = 1;

const char* BUILD_VERSION = "v3.5-002"; // adding build version for debugging. shown on Language choose screen

const unsigned long WEATHER_UPDATE_INTERVAL = 1UL * 60UL * 60UL * 1000UL;
