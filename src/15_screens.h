// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= SCREENS =================
void drawGreetingScreen() {
  u8g2.drawFrame(8, 12, 112, 40);

  // FIX v2.8:
  // Welcome screen supports both languages and uses two centered lines,
  // so text does not go outside the frame.
  if (uiLang == LANG_UA) {
    centerText("Привіт,", 29, u8g2_font_6x12_t_cyrillic);
    centerText("Віталік! :)", 44, u8g2_font_6x12_t_cyrillic);
  } else {
    centerText("Welcome", 29, u8g2_font_6x13_tr);
    centerText("Vitalik! :)", 44, u8g2_font_6x13_tr);
  }

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

void drawUI() {
  u8g2.clearBuffer();

  if (currentScreen == SCREEN_PLAYER) drawPlayerScreen();
  else if (currentScreen == SCREEN_WEATHER) drawWeatherScreen();
  else if (currentScreen == SCREEN_MESSAGE) drawMessageScreen();
  else if (currentScreen == SCREEN_GREETING) drawGreetingScreen();
  else drawClockScreen();

  u8g2.sendBuffer();
}

bool resetComboHeld() {
  return digitalRead(BTN_PLAYER) == LOW && digitalRead(BTN_WEATHER) == LOW;
}
