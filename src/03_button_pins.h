// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= BUTTONS =================
// INPUT_PULLUP: button connects GPIO to GND when pressed.
#define BTN_PLAYER   13
#define BTN_CLOCK    16
#define BTN_WEATHER  17


// Separate service/config reset button, hold 2.5s during boot.
// Do NOT use GPIO192 on classic ESP32: it does not exist.
#define CONFIG_RESET_PIN -1
