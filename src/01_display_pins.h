// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= TRANSPARENT OLED / WAVESHARE 1.51" =================
// OLEG 4.0 Sister: Waveshare 1.51" Transparent OLED, SSD1309, 128x64, 4-wire SPI.
// Kept as software SPI so GPIO19 can be used as OLED CS without VSPI MISO conflict.
#define OLED_CLK 18
#define OLED_DIN 23
#define OLED_CS  19
#define OLED_DC  22
#define OLED_RST 16

U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(
  U8G2_R0,
  /* clock=*/ OLED_CLK,
  /* data=*/ OLED_DIN,
  /* cs=*/ OLED_CS,
  /* dc=*/ OLED_DC,
  /* reset=*/ OLED_RST
);
