// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

#include "21_sleep_kitty_bitmap.h"

// ================= SCREENS =================
void drawGreetingScreen() {
  u8g2.drawFrame(8, 12, 112, 40);

  // v3.5-008: second greeting line is runtime-configurable.
  String welcomeLine = speakerConfig.welcomeText;
  welcomeLine.trim();
  if (welcomeLine.length() == 0) welcomeLine = "OLED BT Speaker";

  if (uiLang == LANG_UA) {
    centerText("Привіт,", 29, u8g2_font_6x12_t_cyrillic);
  } else {
    centerText("Welcome", 29, u8g2_font_6x13_tr);
  }

  // Use Cyrillic-capable font for configured text; ASCII also renders fine.
  centerText(welcomeLine, 44, u8g2_font_6x12_t_cyrillic);

  u8g2.drawHLine(24, 49, 80);
}

void drawClockScreen() {
  drawTopBar();

  centerText(timeStr(), 40, u8g2_font_logisoso26_tn);
  centerText(dateStr(), 52, uiLang == LANG_UA ? u8g2_font_6x12_t_cyrillic : u8g2_font_6x10_tr);

  drawNavBar(SCREEN_CLOCK);
}

void drawPlayerScreen() {
  drawTopBar();

  if (!btConnected) {
    if (uiLang == LANG_UA) {
      centerText("BT не підключено", 34, u8g2_font_6x12_t_cyrillic);
      centerText("Час / Погода OK", 47, u8g2_font_5x8_tr);
    } else {
      centerText("BT not connected", 34, u8g2_font_6x10_tr);
      centerText("Clock / Weather OK", 47, u8g2_font_5x8_tr);
    }

    drawNavBar(SCREEN_PLAYER);
    return;
  }

  // FIX v2.8:
  // Dynamic duration from AVRCP PLAYING_TIME + local elapsed fallback.
  String dur = durationDisplayStr();

  u8g2.setFont(u8g2_font_logisoso18_tn);
  int durW = u8g2.getUTF8Width(dur.c_str());
  int durX = (128 - durW) / 2;
  if (durX < 30) durX = 30;
  u8g2.setCursor(durX, 34);
  u8g2.print(dur);

  // FIX v2.8:
  // New brick equalizer around duration.
  drawBrickEqualizer(durX - 20, durX + durW + 3, 35);

  // Cyrillic-capable metadata lines.
  drawMarqueeLine(title, 45, titleOffset, u8g2_font_6x12_t_cyrillic, 4, 124);
  drawMarqueeLine(artist, 53, artistOffset, u8g2_font_4x6_t_cyrillic, 4, 124);

  drawNavBar(SCREEN_PLAYER);
}

void drawWeatherScreen() {
  drawTopBar();

  drawWeatherIcon(9, 16);
  // OLED v3.5-004: humidity block mirrors the weather icon on the right.
  // Top: big humidity percent. Bottom: label aligned like weatherStateLabel().
  if (weatherHumidity >= 0) {
    String humidityNum = String(weatherHumidity);

    u8g2.setFont(u8g2_font_7x14B_tr);
    int humidityNumW = u8g2.getUTF8Width(humidityNum.c_str());
    int humidityNumX = 111 - humidityNumW / 2;
    if (humidityNumX < 88) humidityNumX = 88;

    u8g2.setCursor(humidityNumX, 32);
    u8g2.print(humidityNum);

    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(humidityNumX + humidityNumW + 1, 27);
    u8g2.print("%");
  } else {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(104, 32);
    u8g2.print("--%");
  }

  u8g2.setFont(uiLang == LANG_UA ? u8g2_font_4x6_t_cyrillic : u8g2_font_5x8_tr);
  const char* humidityLabel = (uiLang == LANG_UA) ? "ВОЛОГІСТЬ" : "HUMID";
  int humidityLabelW = u8g2.getUTF8Width(humidityLabel);
  int humidityLabelX = 113 - humidityLabelW / 2;
  if (humidityLabelX + humidityLabelW > 127) humidityLabelX = 127 - humidityLabelW;
  if (humidityLabelX < 86) humidityLabelX = 86;

  u8g2.setCursor(humidityLabelX, 50);
  u8g2.print(humidityLabel);

  u8g2.setFont(u8g2_font_logisoso18_tn);
  u8g2.setCursor(50, 39);
  u8g2.print(String((int)weatherTemp));

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(80, 27, "C");

  // FIX v2.13:
  // Ukrainian "Відч." needs Cyrillic-capable font.
  if (uiLang == LANG_UA) {
    u8g2.setFont(u8g2_font_4x6_t_cyrillic);
    u8g2.setCursor(48, 50);
    u8g2.print("Відч.");
  } else {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(48, 50);
    u8g2.print("Feels ");
  }

  u8g2.print(String((int)weatherFeels));
  u8g2.print("C");

  // FIX v2.13:
  // Weather state label localized.
  u8g2.setFont(uiLang == LANG_UA ? u8g2_font_4x6_t_cyrillic : u8g2_font_5x8_tr);
  u8g2.setCursor(9, 50);
  u8g2.print(weatherStateLabel());

  drawNavBar(SCREEN_WEATHER);
}


void drawSleepZPixelGlyph(int x, int y, uint8_t scale) {
  uint8_t w = scale * 3;
  u8g2.drawBox(x, y, w, scale);
  u8g2.drawBox(x, y + scale * 4, w, scale);

  for (uint8_t i = 0; i < 3; i++) {
    u8g2.drawBox(x + scale * (2 - i), y + scale * (1 + i), scale, scale);
  }
}

void drawSleepZAnimation() {
  uint8_t frame = (millis() / SLEEP_Z_ANIMATION_MS) % 3;

  // v3.5-012: animate only the sleep letters; the cat bitmap stays static.
  drawSleepZPixelGlyph(57, 17, 1);
  if (frame >= 1) drawSleepZPixelGlyph(66, 12, 1);
  if (frame >= 2) drawSleepZPixelGlyph(76, 5, 2);
}

void drawSleepScreen() {
  u8g2.drawXBMP(0, 0, KITTY_SLEEP_WIDTH, KITTY_SLEEP_HEIGHT, kitty_sleep_128x64);
  drawSleepZAnimation();
}

void drawMessageScreen() {
  u8g2.drawFrame(8, 12, 112, 40);

  if (uiLang == LANG_UA) {
    centerText("Bluetooth", 28, u8g2_font_6x13_tr);
    centerText("не підключено", 42, u8g2_font_6x12_t_cyrillic);
  } else {
    centerText("Bluetooth", 28, u8g2_font_6x13_tr);
    centerText("is not connected", 42, u8g2_font_6x10_tr);
  }

  u8g2.drawHLine(24, 49, 80);
}


void drawVolumeOverlayScreen() {
  const char* title = (uiLang == LANG_UA) ? "Гучність" : "Volume";
  String valueText = String(volumeOverlayPercent) + "%";

  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(1);

  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawFrame(2, 2, 124, 60);

  u8g2.setFont(uiLang == LANG_UA ? u8g2_font_6x12_t_cyrillic : u8g2_font_6x13_tr);
  int titleW = u8g2.getUTF8Width(title);
  u8g2.setCursor((128 - titleW) / 2, 18);
  u8g2.print(title);

  u8g2.drawHLine(14, 24, 100);

  u8g2.setFont(u8g2_font_logisoso18_tn);
  int numberW = u8g2.getUTF8Width(valueText.c_str());
  u8g2.setFont(u8g2_font_6x12_tr);
  int percentW = u8g2.getUTF8Width("%");

  int valueX = (128 - (numberW + percentW + 2)) / 2;

  u8g2.setFont(u8g2_font_logisoso18_tn);
  u8g2.setCursor(valueX, 50);
  u8g2.print(valueText);

  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setCursor(valueX + numberW + 2, 48);
  u8g2.print("%");
}

void drawUI() {
  u8g2.clearBuffer();

  if (volumeOverlayActive) drawVolumeOverlayScreen();
  else if (currentScreen == SCREEN_PLAYER) drawPlayerScreen();
  else if (currentScreen == SCREEN_WEATHER) drawWeatherScreen();
  else if (currentScreen == SCREEN_MESSAGE) drawMessageScreen();
  else if (currentScreen == SCREEN_GREETING) drawGreetingScreen();
  else if (currentScreen == SCREEN_SLEEP) drawSleepScreen();
  else drawClockScreen();

  u8g2.sendBuffer();
}

bool resetComboHeld() {
  return digitalRead(BTN_PLAYER) == LOW && digitalRead(BTN_WEATHER) == LOW;
}
