// RT-003 v5.0 — 1.8" ST7735, 160x128 landscape.
// Proven on the Lolita/ST7735 bench: BLACKTAB, rotation 1, SPI 27 MHz.

constexpr int TFT_SCLK = 18;
constexpr int TFT_MOSI = 23;
constexpr int TFT_CS   = 19;
constexpr int TFT_DC   = 22;
constexpr int TFT_RST  = 16;
constexpr int TFT_MISO = -1;

constexpr uint8_t TFT_ROTATION = 1;
constexpr uint32_t TFT_SPI_HZ = 27000000UL;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
TftUi::Renderer tftUi(tft);
U8G2_FOR_ADAFRUIT_GFX tftText;

void setupDisplay() {
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(TFT_ROTATION);
  tft.setSPISpeed(TFT_SPI_HZ);
  tft.fillScreen(TftUiTheme::BG);
  tftUi.begin();
  tftText.begin(tft);
  tftText.setFontMode(1);
  tftText.setFontDirection(0);
  tftText.setForegroundColor(TftUiTheme::FG);
  tftText.setBackgroundColor(TftUiTheme::BG);
}
