// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= BATTERY ADC =================
#define BATTERY_ADC_PIN 34
const float ADC_REF_VOLTAGE = 3.30;
const float ADC_MAX = 4095.0;
const float DIVIDER_RATIO = 2.0;
const float BATTERY_ABSENT_VOLTAGE = 0.50;
