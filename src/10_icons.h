// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= ICONS =================
void drawBTIconSmall(int x, int y) {
  // FIX v3.0:
  // Smaller Bluetooth icon so it does not touch the topbar line.
  if (!btConnected) {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(x, y + 7, "BT");
    return;
  }

  // Compact 7px high BT rune.
  u8g2.drawLine(x + 3, y + 1, x + 3, y + 8);
  u8g2.drawLine(x + 3, y + 1, x + 7, y + 3);
  u8g2.drawLine(x + 7, y + 3, x + 3, y + 5);
  u8g2.drawLine(x + 3, y + 5, x + 7, y + 7);
  u8g2.drawLine(x + 7, y + 7, x + 3, y + 8);
  u8g2.drawLine(x + 1, y + 3, x + 3, y + 5);
  u8g2.drawLine(x + 1, y + 7, x + 3, y + 5);
}

void drawWiFiIcon(int x, int y, bool connected) {
  // FIX v2.11:
  // Cleaner pixel-art Wi-Fi icon aligned with BT icon.

  // outer arc
  u8g2.drawPixel(x + 0, y + 5);
  u8g2.drawPixel(x + 1, y + 4);
  u8g2.drawPixel(x + 2, y + 3);
  u8g2.drawPixel(x + 6, y + 3);
  u8g2.drawPixel(x + 7, y + 4);
  u8g2.drawPixel(x + 8, y + 5);

  // middle arc
  u8g2.drawPixel(x + 2, y + 6);
  u8g2.drawPixel(x + 3, y + 5);
  u8g2.drawPixel(x + 5, y + 5);
  u8g2.drawPixel(x + 6, y + 6);

  // inner arc
  u8g2.drawPixel(x + 3, y + 7);
  u8g2.drawPixel(x + 4, y + 6);
  u8g2.drawPixel(x + 5, y + 7);

  // center dot
  u8g2.drawPixel(x + 4, y + 9);

  if (!connected) {
    u8g2.drawLine(x + 9, y + 0, x + 0, y + 10);
  }
}

void drawBatteryUnavailableSlash(int x, int y) {
  // One diagonal slash, no X.
  // Final reference: top-right -> bottom-left.
  u8g2.drawLine(x + 20, y - 1, x + 1, y + 9);
}

void drawBatteryIconCompact(int x, int y) {
  u8g2.drawFrame(x, y, 20, 8);
  u8g2.drawBox(x + 20, y + 2, 2, 4);

  if (!batteryPresent || batteryPercent <= 0) {
    drawBatteryUnavailableSlash(x, y);
    return;
  }

  int fill = map(batteryPercent, 0, 100, 0, 18);
  fill = constrain(fill, 0, 18);
  u8g2.drawBox(x + 1, y + 1, fill, 6);
}

void drawSunIcon(int x, int y) {
  u8g2.drawDisc(x + 8, y + 8, 4);
  u8g2.drawLine(x + 8, y + 1, x + 8, y + 3);
  u8g2.drawLine(x + 8, y + 13, x + 8, y + 15);
  u8g2.drawLine(x + 1, y + 8, x + 3, y + 8);
  u8g2.drawLine(x + 13, y + 8, x + 15, y + 8);
  u8g2.drawLine(x + 3, y + 3, x + 5, y + 5);
  u8g2.drawLine(x + 13, y + 3, x + 11, y + 5);
  u8g2.drawLine(x + 3, y + 13, x + 5, y + 11);
  u8g2.drawLine(x + 13, y + 13, x + 11, y + 11);
}

void drawCloudIcon(int x, int y) {
  // Filled cloud icon, not outline.
  u8g2.drawDisc(x + 6,  y + 10, 4);
  u8g2.drawDisc(x + 12, y + 8,  5);
  u8g2.drawDisc(x + 18, y + 11, 4);
  u8g2.drawBox(x + 4, y + 10, 17, 5);
  u8g2.drawHLine(x + 4, y + 15, 17);
}

void drawRainIcon(int x, int y) {
  drawCloudIcon(x, y);
  u8g2.drawLine(x + 6, y + 17, x + 4, y + 21);
  u8g2.drawLine(x + 12, y + 17, x + 10, y + 21);
  u8g2.drawLine(x + 18, y + 17, x + 16, y + 21);
}

void drawSnowIcon(int x, int y) {
  drawCloudIcon(x, y);
  u8g2.drawPixel(x + 6, y + 18);
  u8g2.drawPixel(x + 12, y + 20);
  u8g2.drawPixel(x + 18, y + 18);
}

void drawWeatherIcon(int x, int y) {
  if (weatherState == "SUN") drawSunIcon(x, y);
  else if (weatherState == "RAIN") drawRainIcon(x, y);
  else if (weatherState == "SNOW") drawSnowIcon(x, y);
  else drawCloudIcon(x, y);
}

String weatherStateLabel() {
  // FIX v2.13:
  // Localized short weather state label under icon.
  if (uiLang == LANG_UA) {
    if (weatherState == "SUN") return "ЯСНО";
    if (weatherState == "CLOUD") return "ХМАРНО";
    if (weatherState == "RAIN") return "ДОЩ";
    if (weatherState == "SNOW") return "СНІГ";
    return "ПОГ.";
  }

  return weatherState;
}
