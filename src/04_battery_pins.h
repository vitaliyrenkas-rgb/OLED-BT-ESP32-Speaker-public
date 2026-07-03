// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= BATTERY ADC =================
#define BATTERY_ADC_PIN 34
#define USB_VBUS_ADC_PIN 33
#define VOLUME_ADC_PIN 35
const float ADC_REF_VOLTAGE = 3.30;
const float ADC_MAX = 4095.0;
const float DIVIDER_RATIO = 2.0;
const float USB_VBUS_DIVIDER_RATIO = 2.0;
const float USB_VBUS_PRESENT_VOLTAGE = 3.0;
const float BATTERY_ABSENT_VOLTAGE = 0.50;
const float BATTERY_FULL_VOLTAGE = 4.00;
const float BATTERY_FILTER_FAST_DELTA = 0.18;


// ================= VOLUME POT ADC =================
const unsigned long VOLUME_POT_UPDATE_INTERVAL = 150UL;
const int VOLUME_POT_MIN_VOLUME = 0;
const int VOLUME_POT_MAX_VOLUME = 127;
const int VOLUME_POT_DEADBAND = 5;
const int VOLUME_POT_RAW_DEADBAND = 96;
const int VOLUME_POT_FAST_MOVE_RAW = 900;
