// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= PLAYER HELPERS =================
void drawMarqueeLine(const String &s, int y, int &offset, const uint8_t *font, int left, int right) {
  u8g2.setFont(font);

  int maxW = right - left;
  int w = u8g2.getUTF8Width(s.c_str());

  if (w <= maxW) {
    int x = left + (maxW - w) / 2;
    u8g2.setCursor(x, y);
    u8g2.print(s);
    return;
  }

  if (millis() - lastMarquee > 120) {
    offset--;
    lastMarquee = millis();
    requestRedraw();
  }

  if (offset < -w - 18) offset = maxW;

  u8g2.setCursor(left + offset, y);
  u8g2.print(s);

  u8g2.setCursor(left + offset + w + 18, y);
  u8g2.print(s);
}

void drawBrickEqColumn(int x, int baseY, int bricks, int maxBricks) {
  // FIX v2.8:
  // Brick equalizer, accepted as the new UI direction.
  const int brickW = 3;
  const int brickH = 2;
  const int gap = 1;

  bricks = constrain(bricks, 0, maxBricks);

  for (int i = 0; i < bricks; i++) {
    int y = baseY - i * (brickH + gap);
    u8g2.drawBox(x, y, brickW, brickH);
  }
}

void drawBrickEqualizer(int leftX, int rightX, int baseY) {
  const int maxBricks = 6;

  // FIX v2.11:
  // 3-column EQ per side, driven by real PCM level.
  uint8_t level = audioLevelToBricks();

  if (!playbackActive) level = 0;

  int l1 = level;
  int l2 = level > 1 ? level - 1 : level;
  int l3 = level > 2 ? level - 2 : level;

  int r1 = level > 2 ? level - 2 : level;
  int r2 = level > 1 ? level - 1 : level;
  int r3 = level;

  drawBrickEqColumn(leftX,       baseY, l1, maxBricks);
  drawBrickEqColumn(leftX + 5,   baseY, l2, maxBricks);
  drawBrickEqColumn(leftX + 10,  baseY, l3, maxBricks);

  drawBrickEqColumn(rightX,      baseY, r1, maxBricks);
  drawBrickEqColumn(rightX + 5,  baseY, r2, maxBricks);
  drawBrickEqColumn(rightX + 10, baseY, r3, maxBricks);
}
