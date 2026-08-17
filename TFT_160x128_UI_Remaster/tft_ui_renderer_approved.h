#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <math.h>
#include "ui_theme.h"

// =============================================================
// APPROVED TFT UI RENDERER
// =============================================================
// Source of truth: the user-approved 160x128 mockups from 2026-08-17.
// This is NOT a fresh reinterpretation of the old OLED geometry.
//
// Redraw rule:
// - full clear only when the scene/screen changes;
// - animated/stateful elements redraw only their own dirty rectangles;
// - no full-screen black curtain on EQ / volume / Z animation frames.
// =============================================================

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
  uint8_t eqLevel = 5;       // showroom input 0..7
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
    _text.setFontMode(1); // transparent glyph background
    _text.setFontDirection(0);
    _text.setBackgroundColor(TftUiTheme::BG);
    invalidate();
  }

  void invalidate() {
    _sceneValid = false;
    _cacheValid = false;
  }

  void draw(const Model &m) {
    const bool sceneChanged = !_sceneValid || m.screen != _scene;

    if (sceneChanged) {
      // One full clear is allowed at an actual scene transition.
      _gfx.fillScreen(TftUiTheme::BG);
      drawFullScene(m);
      _scene = m.screen;
      _sceneValid = true;
      _cache = m;
      _cacheValid = true;
      return;
    }

    // Same screen: only dirty regions are touched.
    updateTopBarDirty(m);

    switch (m.screen) {
      case PLAYER:  updatePlayerDirty(m); break;
      case CLOCK:   updateClockDirty(m); break;
      case WEATHER: updateWeatherDirty(m); break;
      case VOLUME:  updateVolumeDirty(m); break;
      case SLEEP:   updateSleepDirty(m); break;
    }

    _cache = m;
    _cacheValid = true;
  }

private:
  Adafruit_GFX &_gfx;
  U8G2_FOR_ADAFRUIT_GFX _text;

  Screen _scene = PLAYER;
  bool _sceneValid = false;
  bool _cacheValid = false;
  Model _cache;

  // ---------------- Text helpers ----------------

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
    const int16_t w = _text.getUTF8Width(s.c_str());
    const int16_t x = left + ((right - left) - w) / 2;
    _text.setCursor(x, baselineY);
    _text.print(s);
  }

  void clearRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    _gfx.fillRect(x, y, w, h, TftUiTheme::BG);
  }

  // ---------------- Scene dispatch ----------------

  void drawFullScene(const Model &m) {
    switch (m.screen) {
      case PLAYER:  drawPlayerFull(m); break;
      case CLOCK:   drawClockFull(m); break;
      case WEATHER: drawWeatherFull(m); break;
      case VOLUME:  drawVolumeFull(m); break;
      case SLEEP:   drawSleepFull(m); break;
    }
  }

  // ---------------- Approved common chrome ----------------

  void drawTopBar(const Model &m) {
    drawBtIcon(4, 3, m.btConnected);
    drawWifiIcon(17, 2, m.wifiConnected);

    centerText(13, m.topTime, u8g2_font_6x12_tf,
               TftUiTheme::FG, 52, 106);

    textAt(110, 13, String(m.temperatureC) + "C",
           u8g2_font_5x8_tf, TftUiTheme::FG);

    drawBatteryStatus(128, 3, m);

    _gfx.drawFastHLine(1, 17, 158, TftUiTheme::LINE);
  }

  void updateTopBarDirty(const Model &m) {
    if (m.screen == VOLUME || m.screen == SLEEP || !_cacheValid) return;

    if (m.btConnected != _cache.btConnected) {
      clearRect(2, 1, 12, 15);
      drawBtIcon(4, 3, m.btConnected);
    }

    if (m.wifiConnected != _cache.wifiConnected) {
      clearRect(15, 1, 25, 15);
      drawWifiIcon(17, 2, m.wifiConnected);
    }

    if (m.topTime != _cache.topTime) {
      clearRect(50, 1, 58, 15);
      centerText(13, m.topTime, u8g2_font_6x12_tf,
                 TftUiTheme::FG, 52, 106);
    }

    if (m.temperatureC != _cache.temperatureC) {
      clearRect(108, 1, 20, 15);
      textAt(110, 13, String(m.temperatureC) + "C",
             u8g2_font_5x8_tf, TftUiTheme::FG);
    }

    if (m.batteryPresent != _cache.batteryPresent ||
        m.batteryCharging != _cache.batteryCharging ||
        m.batteryPercent != _cache.batteryPercent) {
      clearRect(127, 1, 33, 15);
      drawBatteryStatus(128, 3, m);
    }
  }

  void drawNavBar(Screen active) {
    _gfx.drawFastHLine(1, 108, 158, TftUiTheme::LINE);

    drawNavItem(0,   53, "Player",  active == PLAYER);
    drawNavItem(54,  52, "Clock",   active == CLOCK);
    drawNavItem(107, 53, "Weather", active == WEATHER);

    _gfx.drawFastVLine(54, 111, 14, TftUiTheme::DIM);
    _gfx.drawFastVLine(106, 111, 14, TftUiTheme::DIM);
  }

  void drawNavItem(int16_t x, int16_t w, const char *label, bool active) {
    if (active) {
      _gfx.fillRect(x + 5, 111, w - 10, 14, TftUiTheme::FG);
      centerText(123, label, u8g2_font_6x12_tf,
                 TftUiTheme::BG, x, x + w);
    } else {
      centerText(123, label, u8g2_font_6x12_tf,
                 TftUiTheme::FG, x, x + w);
    }
  }

  // ---------------- PLAYER: approved mockup ----------------

  void drawPlayerFull(const Model &m) {
    drawTopBar(m);
    drawPlayerDuration(m);
    drawPlayerEq(m);
    drawPlayerMetadata(m);
    drawNavBar(PLAYER);
  }

  void drawPlayerDuration(const Model &m) {
    centerText(61, m.duration, u8g2_font_logisoso32_tn,
               TftUiTheme::FG, 38, 122);
  }

  void drawPlayerEq(const Model &m) {
    const uint8_t level = m.playbackActive ? constrain(m.eqLevel, 0, 7) : 0;
    drawBrickEq(8, 66, level, false);
    drawBrickEq(123, 66, level, true);
  }

  void drawPlayerMetadata(const Model &m) {
    centerText(83, m.title, u8g2_font_7x14B_tf,
               TftUiTheme::FG, 4, 156);
    centerText(96, m.artist, u8g2_font_5x8_tf,
               TftUiTheme::DIM, 4, 156);
  }

  void updatePlayerDirty(const Model &m) {
    if (!_cacheValid) return;

    if (m.duration != _cache.duration) {
      clearRect(37, 30, 86, 34);
      drawPlayerDuration(m);
    }

    if (m.eqLevel != _cache.eqLevel ||
        m.playbackActive != _cache.playbackActive) {
      clearRect(6, 30, 34, 38);
      clearRect(121, 30, 34, 38);
      drawPlayerEq(m);
    }

    if (m.title != _cache.title || m.artist != _cache.artist) {
      clearRect(3, 69, 154, 31);
      drawPlayerMetadata(m);
    }
  }

  void drawBrickEq(int16_t x0, int16_t baseY, uint8_t level, bool mirror) {
    static const int8_t profile[5] = { -2, 1, 3, 0, -1 };
    constexpr int16_t brickW = 4;
    constexpr int16_t brickH = 2;
    constexpr int16_t gapX = 2;
    constexpr int16_t gapY = 1;
    constexpr int maxRows = 11;

    const int baseRows = map(level, 0, 7, 0, 9);

    for (uint8_t c = 0; c < 5; ++c) {
      const uint8_t pc = mirror ? 4 - c : c;
      const int rows = constrain(baseRows + profile[pc], 0, maxRows);
      const int16_t x = x0 + c * (brickW + gapX);

      for (int r = 0; r < rows; ++r) {
        const int16_t y = baseY - brickH - r * (brickH + gapY);
        _gfx.fillRect(x, y, brickW, brickH,
                      TftUiTheme::eqColorForRow(r, maxRows));
      }
    }
  }

  // ---------------- CLOCK: approved mockup ----------------

  void drawClockFull(const Model &m) {
    drawTopBar(m);
    drawClockBody(m);
    drawNavBar(CLOCK);
  }

  void drawClockBody(const Model &m) {
    centerText(70, m.topTime, u8g2_font_logisoso42_tn,
               TftUiTheme::FG, 18, 142);
    centerText(91, m.date, u8g2_font_7x14B_tf,
               TftUiTheme::FG, 28, 132);
  }

  void updateClockDirty(const Model &m) {
    if (!_cacheValid) return;

    if (m.topTime != _cache.topTime) {
      clearRect(18, 27, 124, 48);
      centerText(70, m.topTime, u8g2_font_logisoso42_tn,
                 TftUiTheme::FG, 18, 142);
    }

    if (m.date != _cache.date) {
      clearRect(27, 76, 106, 20);
      centerText(91, m.date, u8g2_font_7x14B_tf,
                 TftUiTheme::FG, 28, 132);
    }
  }

  // ---------------- WEATHER: approved mockup ----------------

  void drawWeatherFull(const Model &m) {
    drawTopBar(m);
    drawWeatherBody(m);
    drawNavBar(WEATHER);
  }

  void drawWeatherBody(const Model &m) {
    drawWeatherIcon(10, 36, m.weatherState, m.weatherNight);

    drawSignedTemperature(52, 108, 68, m.temperatureC);

    centerText(86, String("Feels ") + m.weatherFeelsC + "C",
               u8g2_font_6x12_tf, TftUiTheme::DIM, 51, 109);

    if (m.humidity >= 0) {
      centerText(67, String(m.humidity), u8g2_font_logisoso24_tn,
                 TftUiTheme::FG, 111, 151);
      textAt(148, 65, "%", u8g2_font_6x12_tf, TftUiTheme::FG);
      centerText(86, "HUMIDITY", u8g2_font_5x8_tf,
                 TftUiTheme::DIM, 109, 160);
    } else {
      centerText(67, "--", u8g2_font_logisoso24_tn,
                 TftUiTheme::DIM, 111, 151);
    }

    centerText(86, weatherLabel(m.weatherState), u8g2_font_6x12_tf,
               TftUiTheme::DIM, 0, 51);
  }

  void updateWeatherDirty(const Model &m) {
    if (!_cacheValid) return;

    if (m.weatherState != _cache.weatherState ||
        m.weatherNight != _cache.weatherNight ||
        m.temperatureC != _cache.temperatureC ||
        m.weatherFeelsC != _cache.weatherFeelsC ||
        m.humidity != _cache.humidity) {
      // Weather changes are slow; redraw only the body, never the whole screen.
      clearRect(0, 19, 160, 88);
      drawWeatherBody(m);
    }
  }

  String weatherLabel(const String &state) {
    if (state == "SUN") return "CLEAR";
    if (state == "RAIN") return "RAIN";
    if (state == "SNOW") return "SNOW";
    return "CLOUD";
  }

  void drawSignedTemperature(int16_t left, int16_t right,
                             int16_t baselineY, int value) {
    const String digits = String(abs(value));
    font(u8g2_font_logisoso32_tn, TftUiTheme::FG);
    const int16_t digitsW = _text.getUTF8Width(digits.c_str());
    const bool hasSign = value != 0;
    const int16_t signW = hasSign ? 11 : 0;
    const int16_t groupW = signW + digitsW;
    int16_t x = left + ((right - left) - groupW) / 2;

    if (hasSign) {
      const int16_t sy = baselineY - 17;
      _gfx.drawFastHLine(x + 1, sy, 8, TftUiTheme::FG);
      if (value > 0) _gfx.drawFastVLine(x + 5, sy - 4, 9, TftUiTheme::FG);
      x += signW;
    }

    _text.setCursor(x, baselineY);
    _text.print(digits);
  }

  // ---------------- VOLUME: approved mockup ----------------

  void drawVolumeFull(const Model &m) {
    _gfx.drawRect(3, 3, 154, 122, TftUiTheme::FG);
    _gfx.drawRect(5, 5, 150, 118, TftUiTheme::DIM);

    centerText(26, "Volume", u8g2_font_7x14B_tf, TftUiTheme::FG);
    _gfx.drawFastHLine(13, 31, 134, TftUiTheme::CYAN);

    drawVolumeValue(m.volumePercent);
  }

  void drawVolumeValue(int percent) {
    const String digits = String(constrain(percent, 0, 100));
    font(u8g2_font_logisoso42_tn, TftUiTheme::FG);
    const int16_t digitsW = _text.getUTF8Width(digits.c_str());

    font(u8g2_font_7x14B_tf, TftUiTheme::FG);
    const int16_t percentW = _text.getUTF8Width("%");

    const int16_t totalW = digitsW + 5 + percentW;
    const int16_t startX = (160 - totalW) / 2;

    font(u8g2_font_logisoso42_tn, TftUiTheme::FG);
    _text.setCursor(startX, 88);
    _text.print(digits);

    textAt(startX + digitsW + 5, 85, "%",
           u8g2_font_7x14B_tf, TftUiTheme::FG);
  }

  void updateVolumeDirty(const Model &m) {
    if (!_cacheValid || m.volumePercent == _cache.volumePercent) return;
    clearRect(34, 40, 92, 55);
    drawVolumeValue(m.volumePercent);
  }

  // ---------------- SLEEP: approved high-res kitty direction ----------------

  void drawSleepFull(const Model &m) {
    drawSleepKitty(18, 48);
    drawSleepZ(m);
  }

  void updateSleepDirty(const Model &m) {
    if (!_cacheValid) return;
    const uint8_t oldFrame = (_cache.nowMs / 450UL) % 4;
    const uint8_t newFrame = (m.nowMs / 450UL) % 4;
    if (oldFrame == newFrame) return;

    clearRect(92, 8, 68, 50);
    drawSleepZ(m);
  }

  void drawSleepZ(const Model &m) {
    const uint8_t frame = (m.nowMs / 450UL) % 4;
    drawZ(95, 49, 1, TftUiTheme::DIM);
    if (frame >= 1) drawZ(110, 34, 2, TftUiTheme::FG);
    if (frame >= 2) drawZ(130, 13, 3, TftUiTheme::FG);
  }

  void drawSleepKitty(int16_t x, int16_t y) {
    // Kept procedural so the approved kitty can be point-tuned without replacing
    // the renderer. This is deliberately separate from the Z dirty region.

    _gfx.fillRoundRect(x + 50, y + 22, 78, 45, 20, TftUiTheme::KITTY_DARK);
    _gfx.fillCircle(x + 104, y + 44, 23, TftUiTheme::KITTY_DARK);
    _gfx.fillCircle(x + 103, y + 44, 13, TftUiTheme::BG);

    _gfx.fillRoundRect(x + 10, y + 10, 66, 48, 16, TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 13, y + 16, x + 16, y - 2, x + 32, y + 13,
                      TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 56, y + 11, x + 69, y - 5, x + 72, y + 21,
                      TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 17, y + 10, x + 18, y + 2, x + 27, y + 12,
                      TftUiTheme::KITTY_PINK);
    _gfx.fillTriangle(x + 59, y + 8, x + 67, y - 1, x + 68, y + 15,
                      TftUiTheme::KITTY_PINK);

    _gfx.fillRoundRect(x + 25, y + 48, 22, 14, 7, TftUiTheme::KITTY_FUR);
    _gfx.fillRoundRect(x + 43, y + 48, 23, 14, 7, TftUiTheme::KITTY_FUR);

    _gfx.drawLine(x + 26, y + 31, x + 31, y + 34, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 31, y + 34, x + 36, y + 31, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 50, y + 31, x + 55, y + 34, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 55, y + 34, x + 60, y + 31, TftUiTheme::KITTY_DARK);

    _gfx.fillTriangle(x + 41, y + 38, x + 47, y + 38, x + 44, y + 42,
                      TftUiTheme::KITTY_PINK);
    _gfx.drawLine(x + 44, y + 42, x + 44, y + 46, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 44, y + 46, x + 39, y + 48, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 44, y + 46, x + 49, y + 48, TftUiTheme::KITTY_DARK);

    for (int i = 0; i < 3; ++i)
      _gfx.drawFastVLine(x + 35 + i * 7, y + 13, 8 - i, TftUiTheme::KITTY_DARK);

    _gfx.drawFastVLine(x + 89, y + 25, 18, TftUiTheme::KITTY_FUR);
    _gfx.drawFastVLine(x + 101, y + 26, 18, TftUiTheme::KITTY_FUR);
    _gfx.drawFastVLine(x + 113, y + 31, 15, TftUiTheme::KITTY_FUR);
  }

  void drawZ(int16_t x, int16_t y, uint8_t scale, uint16_t color) {
    const int16_t w = 5 * scale;
    _gfx.fillRect(x, y, w, scale, color);
    _gfx.fillRect(x, y + 4 * scale, w, scale, color);
    for (uint8_t i = 0; i < 4; ++i) {
      _gfx.fillRect(x + (3 - i) * scale,
                    y + (i + 1) * scale,
                    scale, scale, color);
    }
  }

  // ---------------- High-resolution semantic icons ----------------

  void drawBtIcon(int16_t x, int16_t y, bool connected) {
    if (!connected) {
      textAt(x, y + 10, "BT", u8g2_font_5x8_tf, TftUiTheme::DIM);
      return;
    }

    const uint16_t c = TftUiTheme::BLUE;
    _gfx.drawFastVLine(x + 5, y, 12, c);
    _gfx.drawLine(x + 5, y, x + 10, y + 4, c);
    _gfx.drawLine(x + 10, y + 4, x + 5, y + 7, c);
    _gfx.drawLine(x + 5, y + 7, x + 10, y + 11, c);
    _gfx.drawLine(x + 10, y + 11, x + 5, y + 12, c);
    _gfx.drawLine(x + 1, y + 3, x + 5, y + 7, c);
    _gfx.drawLine(x + 1, y + 10, x + 5, y + 7, c);
  }

  void drawWifiIcon(int16_t x, int16_t y, bool connected) {
    const uint16_t c = connected ? TftUiTheme::CYAN : TftUiTheme::DIM;

    _gfx.drawCircle(x + 8, y + 11, 10, c);
    _gfx.drawCircle(x + 8, y + 11, 6, c);
    _gfx.fillRect(x - 3, y + 12, 23, 10, TftUiTheme::BG);
    _gfx.fillCircle(x + 8, y + 11, 2, c);

    if (!connected)
      _gfx.drawLine(x, y + 1, x + 16, y + 14, TftUiTheme::RED);
  }

  void drawBatteryStatus(int16_t x, int16_t y, const Model &m) {
    // Approved mockup has a compact battery icon plus visible percentage.
    constexpr int16_t bodyW = 13;
    constexpr int16_t bodyH = 9;

    _gfx.drawRect(x, y, bodyW, bodyH, TftUiTheme::FG);
    _gfx.fillRect(x + bodyW, y + 3, 2, 4, TftUiTheme::FG);

    if (m.batteryPresent) {
      const int fill = map(constrain(m.batteryPercent, 0, 100), 0, 100, 0, 9);
      if (fill > 0)
        _gfx.fillRect(x + 2, y + 2, fill, 5,
                      TftUiTheme::batteryColor(m.batteryPercent));

      if (m.batteryCharging) {
        _gfx.drawLine(x + 7, y + 1, x + 5, y + 4, TftUiTheme::YELLOW);
        _gfx.drawLine(x + 5, y + 4, x + 8, y + 4, TftUiTheme::YELLOW);
        _gfx.drawLine(x + 8, y + 4, x + 6, y + 8, TftUiTheme::YELLOW);
      }
    } else {
      _gfx.drawLine(x + 2, y + 7, x + 11, y + 1, TftUiTheme::RED);
    }

    textAt(145, 13,
           m.batteryPresent ? String(constrain(m.batteryPercent, 0, 100)) + "%" : "--%",
           u8g2_font_5x8_tf,
           m.batteryPresent ? TftUiTheme::FG : TftUiTheme::DIM);
  }

  void drawWeatherIcon(int16_t x, int16_t y,
                       const String &state, bool night) {
    if (state == "SUN") {
      if (night) drawMoon(x, y);
      else drawSun(x, y);
    } else if (state == "RAIN") {
      drawCloud(x, y, TftUiTheme::CLOUD);
      for (int i = 0; i < 3; ++i)
        _gfx.drawLine(x + 9 + i * 9, y + 30,
                      x + 6 + i * 9, y + 36,
                      TftUiTheme::CYAN);
    } else if (state == "SNOW") {
      drawCloud(x, y, TftUiTheme::CLOUD);
      for (int i = 0; i < 3; ++i)
        _gfx.fillCircle(x + 8 + i * 10,
                        y + 32 + (i & 1) * 2,
                        1, TftUiTheme::FG);
    } else {
      drawCloud(x, y, TftUiTheme::CLOUD);
    }
  }

  uint16_t gradientColorForY(int16_t localY, int16_t diameter) {
    if (localY < diameter / 3) return TftUiTheme::YELLOW;
    if (localY < (diameter * 2) / 3) return TftUiTheme::LIME;
    return TftUiTheme::CYAN;
  }

  void fillGradientCircle(int16_t cx, int16_t cy, int16_t r) {
    for (int16_t dy = -r; dy <= r; ++dy) {
      const int32_t rr = (int32_t)r * r - (int32_t)dy * dy;
      const int16_t dx = (int16_t)sqrt((double)max<int32_t>(0, rr));
      const uint16_t c = gradientColorForY(dy + r, r * 2 + 1);
      _gfx.drawFastHLine(cx - dx, cy + dy, dx * 2 + 1, c);
    }
  }

  void drawMoon(int16_t x, int16_t y) {
    fillGradientCircle(x + 19, y + 18, 16);
    _gfx.fillCircle(x + 27, y + 11, 16, TftUiTheme::BG);

    _gfx.fillRect(x + 37, y + 6, 2, 2, TftUiTheme::CYAN);
    _gfx.fillRect(x + 42, y + 17, 2, 2, TftUiTheme::CYAN);
    _gfx.fillRect(x + 35, y + 28, 2, 2, TftUiTheme::LIME);
  }

  void drawSun(int16_t x, int16_t y) {
    const uint16_t c = TftUiTheme::YELLOW;
    _gfx.fillCircle(x + 19, y + 18, 10, c);
    _gfx.drawFastVLine(x + 19, y, 6, c);
    _gfx.drawFastVLine(x + 19, y + 31, 6, c);
    _gfx.drawFastHLine(x + 1, y + 18, 6, c);
    _gfx.drawFastHLine(x + 32, y + 18, 6, c);
    _gfx.drawLine(x + 6, y + 5, x + 10, y + 9, c);
    _gfx.drawLine(x + 28, y + 27, x + 32, y + 31, c);
    _gfx.drawLine(x + 32, y + 5, x + 28, y + 9, c);
    _gfx.drawLine(x + 10, y + 27, x + 6, y + 31, c);
  }

  void drawCloud(int16_t x, int16_t y, uint16_t c) {
    _gfx.fillCircle(x + 11, y + 20, 8, c);
    _gfx.fillCircle(x + 22, y + 15, 11, c);
    _gfx.fillCircle(x + 34, y + 21, 8, c);
    _gfx.fillRoundRect(x + 5, y + 20, 37, 11, 5, c);
  }
};

} // namespace TftUi
