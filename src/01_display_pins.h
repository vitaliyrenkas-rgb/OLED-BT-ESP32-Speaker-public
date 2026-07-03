// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= OLED / REAL DEVICE PINS =================
#define OLED_SDA 23
#define OLED_SCL 22 //LoLin ESP32 MicroPython Pin. Different from ESP DevKit

// Your test module behaved as SH1106 in previous hard sketches.
// If the real OLED is SSD1306 and image is shifted/wrong, replace constructor with:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
