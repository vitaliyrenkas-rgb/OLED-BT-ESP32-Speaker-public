#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "ui_theme.h"

// TFT 160x128 visual remaster of the existing OLED UI.
// IMPORTANT: this file contains NO GPIO assignments and NO ST7735 init profile.
// It is deliberately hardware-agnostic until physical wiring is confirmed.

namespace TftUi {

enum Screen : uint8_t {
  PLAYER,
  CLOCK,
  WEATHER,
  VOLUME,
  SLEEP
};

struct Model {
  Screen screen = PLAYER;

  bool btConnected = true;
  bool wifiConnected = true;
  bool batteryPresent = true;
  bool batteryCharging = false;
  int batteryPercent = 82;

  String topTime = "23:47";
  int temperatureC = 21;

  String duration = "02:36";
  String title = "Metallica - One";
  String artist = "...And Justice for All";
  uint8_t eqLevel = 5;       // 0..7
  bool playbackActive = true;

  String date = "SUN 17 AUG";

  String weatherState = "SUN";   // SUN / CLOUD / RAIN / SNOW
  bool weatherNight = true;
  int weatherFeelsC = 19;
  int humidity = 64;

  int volumePercent = 37;

  uint32_t nowMs = 0;
};

class Renderer {
public:
  explicit Renderer(Adafruit_GFX &gfx) : _gfx(gfx) {}

  void begin() {
    _text.begin(_gfx);
    _text.setFontMode(1);
    _text.setFontDirection(0);
    _text.setBackgroundColor(TftUiTheme::BG);
  }

  void draw(const Model &m) {
    _gfx.fillScreen(TftUiTheme::BG);

    switch (m.screen) {
      case PLAYER:  drawPlayer(m); break;
      case CLOCK:   drawClock(m); break;
      case WEATHER: drawWeather(m); break;
      case VOLUME:  drawVolume(m); break;
      case SLEEP:   drawSleep(m); break;
    }
  }

private:
  Adafruit_GFX &_gfx;
  U8G2_FOR_ADAFRUIT_GFX _text;

  void font(const uint8_t *f, uint16_t color = TftUiTheme::FG) {
    _text.setFont(f);
    _text.setForegroundColor(color);
  }

  void textAt(int16_t x, int16_t baselineY, const String &s,
              const uint8_t *f, uint16_t color = TftUiTheme::FG) {
    font(f, color);
    _text.setCursor(x, baselineY);
    _text.print(s);
  }

  void centerText(int16_t baselineY, const String &s,
                  const uint8_t *f, uint16_t color = TftUiTheme::FG,
                  int16_t left = 0, int16_t right = TftUiTheme::WIDTH) {
    font(f, color);
    int16_t w = _text.getUTF8Width(s.c_str());
    int16_t x = left + ((right - left) - w) / 2;
    _text.setCursor(x, baselineY);
    _text.print(s);
  }

  // ---------------- Common chrome ----------------

  void drawTopBar(const Model &m) {
    drawBtIcon(4, 3, m.btConnected);
    drawWifiIcon(22, 3, m.wifiConnected);

    centerText(13, m.topTime, u8g2_font_6x12_tf, TftUiTheme::FG, 50, 105);

    String temp = String(m.temperatureC) + "C";
    textAt(111, 13, temp, u8g2_font_5x8_tf, TftUiTheme::FG);

    drawBattery(136, 4, m);
    _gfx.drawFastHLine(0, TftUiTheme::TOPBAR_BOTTOM, 160, TftUiTheme::LINE);
  }

  void drawNavBar(Screen active) {
    _gfx.drawFastHLine(0, TftUiTheme::NAVBAR_TOP, 160, TftUiTheme::LINE);

    drawNavItem(0,   53, "Player",  active == PLAYER);
    drawNavItem(54,  51, "Clock",   active == CLOCK);
    drawNavItem(106, 54, "Weather", active == WEATHER);
  }

  void drawNavItem(int16_t x, int16_t w, const char *label, bool active) {
    if (active) {
      _gfx.fillRect(x + 2, 110, w - 4, 16, TftUiTheme::FG);
      centerText(122, label, u8g2_font_6x12_tf, TftUiTheme::BG, x, x + w);
    } else {
      centerText(122, label, u8g2_font_6x12_tf, TftUiTheme::FG, x, x + w);
    }

    if (x > 0) _gfx.drawFastVLine(x, 111, 14, TftUiTheme::DIM);
  }

  // ---------------- Player ----------------

  void drawPlayer(const Model &m) {
    drawTopBar(m);

    centerText(61, m.duration, u8g2_font_logisoso32_tn, TftUiTheme::FG, 38, 122);

    uint8_t level = m.playbackActive ? constrain(m.eqLevel, 0, 7) : 0;
    drawBrickEq(8, 70, level, false);
    drawBrickEq(128, 70, level, true);

    // Preserve OLED hierarchy: title first, artist below it.
    centerClippedLine(83, m.title, u8g2_font_7x14B_tf, TftUiTheme::FG, 5, 155);
    centerClippedLine(96, m.artist, u8g2_font_5x8_tf, TftUiTheme::DIM, 5, 155);

    drawNavBar(PLAYER);
  }

  void centerClippedLine(int16_t baselineY, const String &s, const uint8_t *f,
                         uint16_t color, int16_t left, int16_t right) {
    // Deliberately no automatic scrolling here: integration can feed the same
    // titleOffset/artistOffset logic from the OLED firmware later.
    font(f, color);
    int16_t maxW = right - left;
    int16_t w = _text.getUTF8Width(s.c_str());
    int16_t x = (w <= maxW) ? left + (maxW - w) / 2 : left;

    // Adafruit_GFX has no generic clipping region, so truncate by characters
    // conservatively for this standalone renderer.
    if (w <= maxW) {
      _text.setCursor(x, baselineY);
      _text.print(s);
      return;
    }

    String out;
    for (size_t i = 0; i < s.length(); ++i) {
      String candidate = out + s[i];
      if (_text.getUTF8Width(candidate.c_str()) > maxW - 8) break;
      out = candidate;
    }
    out += "~";
    _text.setCursor(left, baselineY);
    _text.print(out);
  }

  void drawBrickEq(int16_t x0, int16_t baseY, uint8_t level, bool mirror) {
    static const int8_t profile[5] = { -2, 0, 2, 0, -1 };
    constexpr int16_t brickW = 4;
    constexpr int16_t brickH = 3;
    constexpr int16_t gapX = 2;
    constexpr int16_t gapY = 2;
    constexpr uint8_t maxRows = 7;

    for (uint8_t c = 0; c < 5; ++c) {
      uint8_t pc = mirror ? 4 - c : c;
      int rows = constrain((int)level + profile[pc], 0, (int)maxRows);
      int16_t x = x0 + c * (brickW + gapX);

      for (int r = 0; r < rows; ++r) {
        int16_t y = baseY - brickH - r * (brickH + gapY);
        _gfx.fillRect(x, y, brickW, brickH,
                      TftUiTheme::eqColorForRow(r, maxRows));
      }
    }
  }

  // ---------------- Clock ----------------

  void drawClock(const Model &m) {
    drawTopBar(m);

    centerText(72, m.topTime, u8g2_font_logisoso42_tn, TftUiTheme::FG);
    centerText(94, m.date, u8g2_font_7x14B_tf, TftUiTheme::FG);

    drawNavBar(CLOCK);
  }

  // ---------------- Weather ----------------

  void drawWeather(const Model &m) {
    drawTopBar(m);

    drawWeatherIcon(13, 37, m.weatherState, m.weatherNight);

    String t = String(m.temperatureC);
    centerText(69, t, u8g2_font_logisoso32_tn, TftUiTheme::FG, 57, 105);
    textAt(99, 51, "C", u8g2_font_6x12_tf, TftUiTheme::DIM);
    centerText(90, String("Feels ") + m.weatherFeelsC + "C",
               u8g2_font_6x12_tf, TftUiTheme::DIM, 51, 112);

    if (m.humidity >= 0) {
      centerText(67, String(m.humidity), u8g2_font_logisoso24_tn,
                 TftUiTheme::FG, 112, 159);
      textAt(148, 56, "%", u8g2_font_6x12_tf, TftUiTheme::FG);
      centerText(89, "HUMID", u8g2_font_5x8_tf, TftUiTheme::DIM, 111, 160);
    } else {
      centerText(67, "--%", u8g2_font_7x14B_tf, TftUiTheme::DIM, 112, 160);
    }

    String label = weatherLabel(m.weatherState);
    centerText(91, label, u8g2_font_6x12_tf, TftUiTheme::DIM, 0, 54);

    drawNavBar(WEATHER);
  }

  String weatherLabel(const String &state) {
    if (state == "SUN") return "CLEAR";
    if (state == "RAIN") return "RAIN";
    if (state == "SNOW") return "SNOW";
    return "CLOUD";
  }

  // ---------------- Volume ----------------

  void drawVolume(const Model &m) {
    _gfx.drawRect(2, 2, 156, 124, TftUiTheme::FG);
    _gfx.drawRect(5, 5, 150, 118, TftUiTheme::DIM);

    centerText(27, "Volume", u8g2_font_7x14B_tf, TftUiTheme::FG);
    _gfx.drawFastHLine(14, 34, 132, TftUiTheme::CYAN);

    centerText(88, String(m.volumePercent), u8g2_font_logisoso42_tn,
               TftUiTheme::FG, 30, 118);
    textAt(111, 86, "%", u8g2_font_7x14B_tf, TftUiTheme::FG);

    int barW = map(constrain(m.volumePercent, 0, 100), 0, 100, 0, 132);
    _gfx.drawRect(14, 103, 132, 7, TftUiTheme::DIM);
    if (barW > 2) _gfx.fillRect(15, 104, max(0, barW - 2), 5, TftUiTheme::CYAN);
  }

  // ---------------- Sleep ----------------

  void drawSleep(const Model &m) {
    // High-resolution colored reinterpretation of the same sleep-kitty scene.
    // Kept procedural on purpose so it remains editable without replacing a
    // 40KB bitmap every time we tweak ears/eyes/body proportions.
    drawSleepKitty(18, 48);

    uint8_t frame = (m.nowMs / 450UL) % 4;
    drawZ(95, 49, 1, TftUiTheme::DIM);
    if (frame >= 1) drawZ(110, 34, 2, TftUiTheme::FG);
    if (frame >= 2) drawZ(130, 13, 3, TftUiTheme::FG);
  }

  void drawSleepKitty(int16_t x, int16_t y) {
    // Body / curled tail.
    _gfx.fillRoundRect(x + 50, y + 22, 78, 45, 20, TftUiTheme::KITTY_DARK);
    _gfx.fillCircle(x + 104, y + 44, 23, TftUiTheme::KITTY_DARK);
    _gfx.fillCircle(x + 103, y + 44, 13, TftUiTheme::BG);

    // Head.
    _gfx.fillRoundRect(x + 10, y + 10, 66, 48, 16, TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 13, y + 16, x + 16, y - 2, x + 32, y + 13,
                      TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 56, y + 11, x + 69, y - 5, x + 72, y + 21,
                      TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 17, y + 10, x + 18, y + 2, x + 27, y + 12,
                      TftUiTheme::KITTY_PINK);
    _gfx.fillTriangle(x + 59, y + 8, x + 67, y - 1, x + 68, y + 15,
                      TftUiTheme::KITTY_PINK);

    // Forepaws.
    _gfx.fillRoundRect(x + 25, y + 48, 22, 14, 7, TftUiTheme::KITTY_FUR);
    _gfx.fillRoundRect(x + 43, y + 48, 23, 14, 7, TftUiTheme::KITTY_FUR);

    // Closed eyes.
    _gfx.drawLine(x + 26, y + 31, x + 31, y + 34, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 31, y + 34, x + 36, y + 31, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 50, y + 31, x + 55, y + 34, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 55, y + 34, x + 60, y + 31, TftUiTheme::KITTY_DARK);

    // Nose + mouth.
    _gfx.fillTriangle(x + 41, y + 38, x + 47, y + 38, x + 44, y + 42,
                      TftUiTheme::KITTY_PINK);
    _gfx.drawLine(x + 44, y + 42, x + 44, y + 46, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 44, y + 46, x + 39, y + 48, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 44, y + 46, x + 49, y + 48, TftUiTheme::KITTY_DARK);

    // Tabby stripes, kept subtle.
    for (int i = 0; i < 3; ++i) {
      _gfx.drawFastVLine(x + 35 + i * 7, y + 13, 8 - i, TftUiTheme::KITTY_DARK);
    }
    _gfx.drawFastVLine(x + 89, y + 25, 18, TftUiTheme::KITTY_FUR);
    _gfx.drawFastVLine(x + 101, y + 26, 18, TftUiTheme::KITTY_FUR);
    _gfx.drawFastVLine(x + 113, y + 31, 15, TftUiTheme::KITTY_FUR);
  }

  void drawZ(int16_t x, int16_t y, uint8_t scale, uint16_t color) {
    int16_t w = 5 * scale;
    _gfx.fillRect(x, y, w, scale, color);
    _gfx.fillRect(x, y + 4 * scale, w, scale, color);
    for (uint8_t i = 0; i < 4; ++i) {
      _gfx.fillRect(x + (3 - i) * scale, y + (i + 1) * scale,
                    scale, scale, color);
    }
  }

  // ---------------- Icons ----------------

  void drawBtIcon(int16_t x, int16_t y, bool connected) {
    if (!connected) {
      textAt(x, y + 10, "BT", u8g2_font_5x8_tf, TftUiTheme::DIM);
      return;
    }

    uint16_t c = TftUiTheme::BLUE;
    _gfx.drawFastVLine(x + 5, y, 12, c);
    _gfx.drawLine(x + 5, y, x + 10, y + 4, c);
    _gfx.drawLine(x + 10, y + 4, x + 5, y + 7, c);
    _gfx.drawLine(x + 5, y + 7, x + 10, y + 11, c);
    _gfx.drawLine(x + 10, y + 11, x + 5, y + 12, c);
    _gfx.drawLine(x + 1, y + 3, x + 5, y + 7, c);
    _gfx.drawLine(x + 1, y + 10, x + 5, y + 7, c);
  }

  void drawWifiIcon(int16_t x, int16_t y, bool connected) {
    uint16_t c = connected ? TftUiTheme::CYAN : TftUiTheme::DIM;
    _gfx.drawCircle(x + 7, y + 11, 1, c);
    _gfx.drawCircle(x + 7, y + 11, 5, c);
    _gfx.drawCircle(x + 7, y + 11, 9, c);
    // hide lower halves to create Wi-Fi arcs
    _gfx.fillRect(x - 3, y + 12, 22, 10, TftUiTheme::BG);
    _gfx.fillCircle(x + 7, y + 11, 2, c);

    if (!connected) {
      _gfx.drawLine(x, y + 1, x + 14, y + 14, TftUiTheme::RED);
    }
  }

  void drawBattery(int16_t x, int16_t y, const Model &m) {
    _gfx.drawRect(x, y, 19, 10, TftUiTheme::FG);
    _gfx.fillRect(x + 19, y + 3, 2, 4, TftUiTheme::FG);

    if (!m.batteryPresent) {
      _gfx.drawLine(x + 2, y + 8, x + 16, y + 1, TftUiTheme::RED);
      return;
    }

    int fill = map(constrain(m.batteryPercent, 0, 100), 0, 100, 0, 15);
    if (fill > 0) {
      _gfx.fillRect(x + 2, y + 2, fill, 6,
                    TftUiTheme::batteryColor(m.batteryPercent));
    }

    if (m.batteryCharging) {
      // Tiny lightning overlay, same semantic role as the OLED charging state.
      _gfx.drawLine(x + 10, y + 1, x + 7, y + 5, TftUiTheme::YELLOW);
      _gfx.drawLine(x + 7, y + 5, x + 10, y + 5, TftUiTheme::YELLOW);
      _gfx.drawLine(x + 10, y + 5, x + 7, y + 9, TftUiTheme::YELLOW);
    }
  }

  void drawWeatherIcon(int16_t x, int16_t y, const String &state, bool night) {
    if (state == "SUN") {
      if (night) drawMoon(x, y);
      else drawSun(x, y);
    } else if (state == "RAIN") {
      drawCloud(x, y, TftUiTheme::CLOUD);
      for (int i = 0; i < 3; ++i)
        _gfx.drawLine(x + 9 + i * 8, y + 28, x + 6 + i * 8, y + 34,
                      TftUiTheme::CYAN);
    } else if (state == "SNOW") {
      drawCloud(x, y, TftUiTheme::CLOUD);
      for (int i = 0; i < 3; ++i)
        _gfx.fillCircle(x + 8 + i * 9, y + 31 + (i & 1) * 2, 1, TftUiTheme::FG);
    } else {
      drawCloud(x, y, TftUiTheme::CLOUD);
    }
  }

  void drawSun(int16_t x, int16_t y) {
    uint16_t c = TftUiTheme::YELLOW;
    _gfx.fillCircle(x + 18, y + 17, 8, c);
    _gfx.drawFastVLine(x + 18, y + 1, 6, c);
    _gfx.drawFastVLine(x + 18, y + 28, 6, c);
    _gfx.drawFastHLine(x + 2, y + 17, 6, c);
    _gfx.drawFastHLine(x + 29, y + 17, 6, c);
    _gfx.drawLine(x + 6, y + 5, x + 10, y + 9, c);
    _gfx.drawLine(x + 26, y + 25, x + 30, y + 29, c);
    _gfx.drawLine(x + 30, y + 5, x + 26, y + 9, c);
    _gfx.drawLine(x + 10, y + 25, x + 6, y + 29, c);
  }

  void drawMoon(int16_t x, int16_t y) {
    _gfx.fillCircle(x + 17, y + 17, 13, TftUiTheme::LIME);
    _gfx.fillCircle(x + 24, y + 12, 13, TftUiTheme::BG);
    _gfx.fillRect(x + 33, y + 5, 2, 2, TftUiTheme::CYAN);
    _gfx.fillRect(x + 37, y + 13, 2, 2, TftUiTheme::CYAN);
    _gfx.fillRect(x + 31, y + 24, 2, 2, TftUiTheme::CYAN);
  }

  void drawCloud(int16_t x, int16_t y, uint16_t c) {
    _gfx.fillCircle(x + 11, y + 19, 8, c);
    _gfx.fillCircle(x + 21, y + 15, 10, c);
    _gfx.fillCircle(x + 31, y + 20, 7, c);
    _gfx.fillRoundRect(x + 6, y + 19, 31, 10, 5, c);
  }
};

} // namespace TftUi
