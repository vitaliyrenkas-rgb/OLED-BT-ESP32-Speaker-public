// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= COMMON UI =================
void drawTopBar() {
  drawBTIconSmall(2, 1);
  drawWiFiIcon(17, -1, wifiLastSyncOk); // FIX v2.8: smaller icon now fits above topbar line

  u8g2.setFont(u8g2_font_6x10_tr);
  String topTime = timeStr();
  int timeW = u8g2.getUTF8Width(topTime.c_str());
  u8g2.setCursor((128 - timeW) / 2 - 4, 9);
  u8g2.print(topTime);

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(80, 9);
  u8g2.print(String((int)weatherTemp));
  u8g2.print("C");

  drawBatteryIconCompact(104, 2);
  u8g2.drawHLine(0, 12, 128);
}

void drawNavItem(int x, int w, const char* label, bool active) {
  if (active) {
    u8g2.drawBox(x, 55, w, 9);
    u8g2.setDrawColor(0);
  }

  u8g2.setFont(uiLang == LANG_UA ? u8g2_font_6x12_t_cyrillic : u8g2_font_5x8_tr);
  int tw = u8g2.getUTF8Width(label);
  int tx = x + (w - tw) / 2;
  if (tx < x) tx = x;
  u8g2.setCursor(tx, 63);
  u8g2.print(label);

  if (active) u8g2.setDrawColor(1);
}

void drawNavBar(ScreenMode active) {
  u8g2.drawHLine(0, 54, 128);

  if (uiLang == LANG_UA) {
    drawNavItem(0, 42, "Плеєр", active == SCREEN_PLAYER);
    drawNavItem(43, 39, "ЧАС", active == SCREEN_CLOCK);
    drawNavItem(83, 45, "Погода", active == SCREEN_WEATHER);
  } else {
    drawNavItem(0, 42, "Player", active == SCREEN_PLAYER);
    drawNavItem(43, 39, "Clock", active == SCREEN_CLOCK);
    drawNavItem(83, 45, "Weather", active == SCREEN_WEATHER);
  }
}
