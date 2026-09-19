#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <math.h>
#include "ui_theme.h"
#include "kitty_approved_asset.h"

namespace TftUi {

enum Screen : uint8_t { PLAYER, CLOCK, WEATHER, VOLUME, SLEEP };

struct Model {
  Screen screen = PLAYER;
  bool ukrainian = true;
  bool btConnected = true;
  bool wifiConnected = true;
  bool batteryPresent = true;
  bool batteryCharging = false;
  int batteryPercent = 82;
  int batteryIconPercent = 82;
  String topTime = "23:47";
  int temperatureC = 21;
  String duration = "02:36";
  String title = "Enjoy the Silence";
  String artist = "Depeche Mode";
  uint8_t eqBands[4] = {8, 5, 10, 6};
  bool playbackActive = true;
  String weekday = "СЕРЕДА";
  String date = "16 ВЕРЕСНЯ 2026";
  String weatherState = "SUN";
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
    invalidate();
  }

  void invalidate() {
    _sceneValid = false;
    _cacheValid = false;
  }

  void draw(const Model &m) {
    const bool enteringPlayer = m.screen == PLAYER &&
                                (!_sceneValid || _scene != PLAYER);
    const bool titleChanged = !_cacheValid || m.title != _cache.title;
    if (m.screen == PLAYER && (enteringPlayer || titleChanged)) {
      _titleMarqueeStartedAtMs = m.nowMs;
    }

    const bool localeChanged = _cacheValid && m.ukrainian != _cache.ukrainian;
    const bool sceneChanged = !_sceneValid || m.screen != _scene || localeChanged;
    if (sceneChanged) {
      drawSceneBackground(m.screen);
      drawFullScene(m);
      _scene = m.screen;
      _sceneValid = true;
      _cache = m;
      _cacheValid = true;
      return;
    }

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
  uint32_t _titleMarqueeStartedAtMs = 0;

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

  bool usesUiGradient(Screen screen) const {
    return screen == PLAYER || screen == CLOCK || screen == WEATHER;
  }

  void drawSceneBackground(Screen screen) {
    if (!usesUiGradient(screen)) {
      _gfx.fillScreen(TftUiTheme::BG);
      return;
    }
    for (int16_t y = 0; y < TftUiTheme::HEIGHT; ++y) {
      _gfx.drawFastHLine(0, y, TftUiTheme::WIDTH,
                         TftUiTheme::backgroundColorForY(y));
    }
  }

  void clearRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!usesUiGradient(_scene)) {
      _gfx.fillRect(x, y, w, h, TftUiTheme::BG);
      return;
    }
    const int16_t y0 = y < 0 ? 0 : y;
    const int16_t rawY1 = y + h;
    const int16_t y1 = rawY1 > TftUiTheme::HEIGHT ? TftUiTheme::HEIGHT : rawY1;
    const int16_t x0 = x < 0 ? 0 : x;
    const int16_t rawX1 = x + w;
    const int16_t x1 = rawX1 > TftUiTheme::WIDTH ? TftUiTheme::WIDTH : rawX1;
    const int16_t clippedW = x1 - x0;
    if (clippedW <= 0) return;
    for (int16_t yy = y0; yy < y1; ++yy) {
      _gfx.drawFastHLine(x0, yy, clippedW,
                         TftUiTheme::backgroundColorForY(yy));
    }
  }

  enum SegmentVariant : uint8_t { SEGMENT_NORMAL, SEGMENT_LARGE };

  uint8_t segmentMask(char ch) const {
    // Bits: A B C D E F G.
    static const uint8_t DIGITS[10] = {
      0x3F, 0x06, 0x5B, 0x4F, 0x66,
      0x6D, 0x7D, 0x07, 0x7F, 0x6F
    };
    if (ch >= '0' && ch <= '9') return DIGITS[ch - '0'];
    if (ch == '-') return 0x40;
    return 0;
  }

  int16_t segmentDigitStep(SegmentVariant variant) const {
    return variant == SEGMENT_LARGE ? 17 : 15;
  }

  int16_t segmentColonStep(SegmentVariant variant) const {
    return variant == SEGMENT_LARGE ? 6 : 5;
  }

  int16_t segmentStringWidth(const String &value, SegmentVariant variant) const {
    if (value.length() == 0) return 0;
    int16_t width = 0;
    for (uint16_t i = 0; i < value.length(); ++i) {
      width += value[i] == ':' ? segmentColonStep(variant)
                               : segmentDigitStep(variant);
    }
    return width - 2;
  }

  void drawSegmentDigit(char ch, int16_t x, int16_t y,
                        SegmentVariant variant, uint16_t color) {
    const uint8_t mask = segmentMask(ch);
    const bool large = variant == SEGMENT_LARGE;
    const int16_t hLen = large ? 11 : 9;
    const int16_t vLen = large ? 13 : 11;
    const int16_t rightX = large ? 13 : 11;
    const int16_t midY = large ? 15 : 13;
    const int16_t lowerY = large ? 17 : 15;
    const int16_t bottomY = large ? 30 : 26;

    if (ch == '-') {
      _gfx.fillRect(x + 2, y + midY, hLen, 2, color);
      return;
    }

    const uint16_t off = TftUiTheme::SEGMENT_OFF;
    _gfx.fillRect(x + 2, y, hLen, 2, (mask & 0x01) ? color : off);
    _gfx.fillRect(x + rightX, y + 2, 2, vLen, (mask & 0x02) ? color : off);
    _gfx.fillRect(x + rightX, y + lowerY, 2, vLen, (mask & 0x04) ? color : off);
    _gfx.fillRect(x + 2, y + bottomY, hLen, 2, (mask & 0x08) ? color : off);
    _gfx.fillRect(x, y + lowerY, 2, vLen, (mask & 0x10) ? color : off);
    _gfx.fillRect(x, y + 2, 2, vLen, (mask & 0x20) ? color : off);
    _gfx.fillRect(x + 2, y + midY, hLen, 2, (mask & 0x40) ? color : off);
  }

  void drawSegmentString(const String &value, int16_t x, int16_t y,
                         SegmentVariant variant, uint16_t color,
                         bool showColon = true) {
    for (uint16_t i = 0; i < value.length(); ++i) {
      const char ch = value[i];
      if (ch == ':') {
        if (showColon) {
          const int16_t top = variant == SEGMENT_LARGE ? 9 : 7;
          const int16_t bottom = variant == SEGMENT_LARGE ? 21 : 17;
          _gfx.fillRect(x + 1, y + top, 2, 2, color);
          _gfx.fillRect(x + 1, y + bottom, 2, 2, color);
        }
        x += segmentColonStep(variant);
      } else {
        drawSegmentDigit(ch, x, y, variant, color);
        x += segmentDigitStep(variant);
      }
    }
  }

  void drawSegmentCentered(const String &value, int16_t centerX, int16_t y,
                           SegmentVariant variant, uint16_t color,
                           bool showColon = true) {
    const int16_t width = segmentStringWidth(value, variant);
    drawSegmentString(value, centerX - width / 2, y, variant, color, showColon);
  }

  void drawLargeC(int16_t x, int16_t y, uint16_t color) {
    _gfx.fillRect(x + 2, y, 9, 3, color);
    _gfx.fillRect(x, y + 2, 3, 16, color);
    _gfx.fillRect(x + 2, y + 17, 9, 3, color);
  }

  void drawFullScene(const Model &m) {
    switch (m.screen) {
      case PLAYER:  drawPlayerFull(m); break;
      case CLOCK:   drawClockFull(m); break;
      case WEATHER: drawWeatherFull(m); break;
      case VOLUME:  drawVolumeFull(m); break;
      case SLEEP:   drawSleepFull(m); break;
    }
  }

  void drawTopBar(const Model &m) {
    drawBtIcon(3, 4, m.btConnected);
    drawWifiIcon(15, 2, m.wifiConnected);
    drawTopBarCenter(m);
    drawBatteryStatus(116, 4, m);
    _gfx.drawFastHLine(0, 17, 160, TftUiTheme::LINE);
  }

  void drawTopBarCenter(const Model &m) {
    const String temperature = String(m.temperatureC) + "C";
    font(u8g2_font_5x8_tf, TftUiTheme::FG);
    const int16_t timeW = _text.getUTF8Width(m.topTime.c_str());
    const int16_t temperatureW = _text.getUTF8Width(temperature.c_str());
    constexpr int16_t gap = 4;
    const int16_t startX = (TftUiTheme::WIDTH - timeW - gap - temperatureW) / 2;

    textAt(startX, 12, m.topTime, u8g2_font_5x8_tf, TftUiTheme::FG);
    textAt(startX + timeW + gap, 12, temperature,
           u8g2_font_5x8_tf, TftUiTheme::ORANGE);
  }

  void updateTopBarDirty(const Model &m) {
    if (m.screen == VOLUME || m.screen == SLEEP || !_cacheValid) return;
    if (m.btConnected != _cache.btConnected) {
      clearRect(1, 1, 12, 15);
      drawBtIcon(3, 4, m.btConnected);
    }
    if (m.wifiConnected != _cache.wifiConnected) {
      clearRect(13, 1, 17, 15);
      drawWifiIcon(15, 2, m.wifiConnected);
    }
    if (m.topTime != _cache.topTime ||
        m.temperatureC != _cache.temperatureC) {
      clearRect(31, 1, 81, 15);
      drawTopBarCenter(m);
    }
    if (m.batteryPresent != _cache.batteryPresent ||
        m.batteryCharging != _cache.batteryCharging ||
        m.batteryPercent != _cache.batteryPercent ||
        m.batteryIconPercent != _cache.batteryIconPercent) {
      clearRect(112, 1, 48, 15);
      drawBatteryStatus(116, 4, m);
    }
  }

  void drawNavBar(const Model &m) {
    _gfx.drawFastHLine(1, 108, 158, TftUiTheme::LINE);
    drawNavItem(0, 53, m.ukrainian ? "Плеєр" : "Player",
                m.screen == PLAYER);
    drawNavItem(54, 52, m.ukrainian ? "Годинник" : "Clock",
                m.screen == CLOCK);
    drawNavItem(107, 53, m.ukrainian ? "Погода" : "Weather",
                m.screen == WEATHER);
    _gfx.drawFastVLine(54, 111, 14, TftUiTheme::DIM);
    _gfx.drawFastVLine(106, 111, 14, TftUiTheme::DIM);
  }

  void drawNavItem(int16_t x, int16_t w, const char *label, bool active) {
    constexpr int16_t topY = 110;
    constexpr int16_t height = 17;
    const uint16_t top = active ? TftUiTheme::NAV_ACTIVE_TOP
                                : TftUiTheme::NAV_IDLE_TOP;
    const uint16_t bottom = active ? TftUiTheme::NAV_ACTIVE_BOTTOM
                                   : TftUiTheme::NAV_IDLE_BOTTOM;

    for (int16_t row = 0; row < height; ++row) {
      const uint16_t color = TftUiTheme::blend565(
        top, bottom, row, height - 1);
      _gfx.drawFastHLine(x + 2, topY + row, w - 4, color);
    }

    if (active) {
      _gfx.drawRect(x + 1, topY - 1, w - 2, height + 1,
                    TftUiTheme::NAV_ACTIVE_BORDER);
    }

    // Keep navigation text transparent and bright over the button gradient.
    // Do not use black/inverted glyphs here: they became square blocks on the
    // physical display instead of a readable active label.
    _text.setFontMode(1);
    font(u8g2_font_5x8_t_cyrillic,
         active ? TftUiTheme::FG : TftUiTheme::DIM);
    const int16_t labelW = _text.getUTF8Width(label);
    const int16_t labelX = x + (w - labelW) / 2;
    _text.setCursor(labelX, 122);
    _text.print(label);
  }

  void drawPlayerFull(const Model &m) {
    drawTopBar(m);
    drawPlayerDuration(m);
    drawPlayerEq(m);
    drawPlayerMetadata(m);
    drawNavBar(m);
  }

  void drawPlayerDuration(const Model &m) {
    drawSegmentCentered(m.duration, 80, 31, SEGMENT_NORMAL, TftUiTheme::FG);
  }

  void drawPlayerEq(const Model &m) {
    drawBrickEq(19, 75, m.eqBands, m.playbackActive, false);
    drawBrickEq(115, 75, m.eqBands, m.playbackActive, true);
  }

  void drawPlayerMetadata(const Model &m) {
    drawPlayerTitle(m);
    centerText(100, m.artist, u8g2_font_4x6_t_cyrillic,
               TftUiTheme::CYAN, 4, 156);
  }

  int16_t playerTitleWidth(const Model &m) {
    font(u8g2_font_6x12_t_cyrillic, TftUiTheme::FG);
    return _text.getUTF8Width(m.title.c_str());
  }

  int16_t playerTitleMarqueeOffset(const Model &m, int16_t titleW) const {
    constexpr int16_t availableW = 152;
    constexpr int16_t gap = 18;
    constexpr uint32_t initialHoldMs = 900UL;
    constexpr uint32_t pixelStepMs = 80UL;
    if (titleW <= availableW) return 0;

    const uint32_t elapsed = m.nowMs - _titleMarqueeStartedAtMs;
    if (elapsed < initialHoldMs) return 0;
    return ((elapsed - initialHoldMs) / pixelStepMs) % (titleW + gap);
  }

  void drawPlayerTitle(const Model &m) {
    constexpr int16_t left = 4;
    constexpr int16_t right = 156;
    constexpr int16_t gap = 18;
    const int16_t titleW = playerTitleWidth(m);
    if (titleW <= right - left) {
      centerText(89, m.title, u8g2_font_6x12_t_cyrillic,
                 TftUiTheme::FG, left, right);
      return;
    }

    const int16_t offset = playerTitleMarqueeOffset(m, titleW);
    textAt(left - offset, 89, m.title,
           u8g2_font_6x12_t_cyrillic, TftUiTheme::FG);
    textAt(left - offset + titleW + gap, 89, m.title,
           u8g2_font_6x12_t_cyrillic, TftUiTheme::FG);
  }

  void updatePlayerDirty(const Model &m) {
    if (!_cacheValid) return;
    if (m.duration != _cache.duration) {
      clearRect(47, 29, 66, 32);
      drawPlayerDuration(m);
    }
    bool eqChanged = m.playbackActive != _cache.playbackActive;
    for (uint8_t band = 0; band < 4 && !eqChanged; ++band) {
      eqChanged = m.eqBands[band] != _cache.eqBands[band];
    }
    if (eqChanged) {
      clearRect(18, 27, 29, 49);
      clearRect(114, 27, 29, 49);
      drawPlayerEq(m);
    }
    if (m.title != _cache.title || m.artist != _cache.artist) {
      clearRect(0, 76, 160, 30);
      drawPlayerMetadata(m);
    } else {
      const int16_t titleW = playerTitleWidth(m);
      if (playerTitleMarqueeOffset(m, titleW) !=
          playerTitleMarqueeOffset(_cache, titleW)) {
        clearRect(0, 77, 160, 14);
        drawPlayerTitle(m);
      }
    }
  }

  void drawBrickEq(int16_t x0, int16_t baseY, const uint8_t levels[4],
                   bool active, bool mirror) {
    constexpr int16_t brickW = 5;
    constexpr int16_t brickH = 3;
    constexpr int16_t gapX = 2;
    constexpr int16_t gapY = 1;
    constexpr int maxRows = 12;
    for (uint8_t c = 0; c < 4; ++c) {
      const uint8_t band = mirror ? 3 - c : c;
      const int rows = active ? constrain(levels[band], 0, maxRows) : 0;
      const int16_t x = x0 + c * (brickW + gapX);
      for (int r = 0; r < maxRows; ++r) {
        const int16_t y = baseY - brickH - r * (brickH + gapY);
        const uint16_t color = r < rows
          ? TftUiTheme::eqColorForRow(r, maxRows)
          : TftUiTheme::SEGMENT_OFF;
        _gfx.fillRect(x, y, brickW, brickH, color);
      }
    }
  }

  void drawClockFull(const Model &m) {
    drawTopBar(m);
    drawClockBody(m);
    drawNavBar(m);
  }

  bool clockColonOn(const Model &m) const {
    return ((m.nowMs / 500UL) & 1U) == 0;
  }

  void drawClockTime(const Model &m) {
    drawSegmentCentered(m.topTime, 80, 28, SEGMENT_LARGE,
                        TftUiTheme::FG, clockColonOn(m));
  }

  void drawClockBody(const Model &m) {
    drawClockTime(m);
    centerText(76, m.weekday, u8g2_font_6x12_t_cyrillic,
               TftUiTheme::CYAN, 4, 156);
    centerText(94, m.date, u8g2_font_6x12_t_cyrillic,
               TftUiTheme::FG, 4, 156);
  }

  void updateClockDirty(const Model &m) {
    if (!_cacheValid) return;
    if (m.topTime != _cache.topTime || clockColonOn(m) != clockColonOn(_cache)) {
      clearRect(43, 26, 74, 36);
      drawClockTime(m);
    }
    if (m.weekday != _cache.weekday || m.date != _cache.date) {
      clearRect(3, 64, 154, 36);
      centerText(76, m.weekday, u8g2_font_6x12_t_cyrillic,
                 TftUiTheme::CYAN, 4, 156);
      centerText(94, m.date, u8g2_font_6x12_t_cyrillic,
                 TftUiTheme::FG, 4, 156);
    }
  }

  void drawWeatherFull(const Model &m) {
    drawTopBar(m);
    drawWeatherBody(m);
    drawNavBar(m);
  }

  void drawWeatherBody(const Model &m) {
    drawWeatherIcon(9, 29, m.weatherState, m.weatherNight);
    drawWeatherTemperature(m.temperatureC);

    centerText(75, weatherLabel(m.weatherState, m.ukrainian),
               u8g2_font_6x12_t_cyrillic, TftUiTheme::CYAN, 3, 157);

    const String feels = String(m.ukrainian ? "Відчувається як " : "Feels like ") +
                         m.weatherFeelsC + "C";
    centerText(89, feels, u8g2_font_6x12_t_cyrillic,
               TftUiTheme::FG, 3, 157);

    const String humidity = m.humidity >= 0
      ? String(m.ukrainian ? "ВОЛОГІСТЬ " : "HUMIDITY ") + m.humidity + "%"
      : String(m.ukrainian ? "ВОЛОГІСТЬ --%" : "HUMIDITY --%");
    centerText(103, humidity, u8g2_font_6x12_t_cyrillic,
               TftUiTheme::CYAN, 3, 157);
  }

  void updateWeatherDirty(const Model &m) {
    if (!_cacheValid) return;
    if (m.weatherState != _cache.weatherState ||
        m.weatherNight != _cache.weatherNight ||
        m.ukrainian != _cache.ukrainian ||
        m.temperatureC != _cache.temperatureC ||
        m.weatherFeelsC != _cache.weatherFeelsC ||
        m.humidity != _cache.humidity) {
      clearRect(0, 19, 160, 88);
      drawWeatherBody(m);
    }
  }

  String weatherLabel(const String &state, bool ukrainian) {
    if (ukrainian) {
      if (state == "SUN") return "ЯСНО";
      if (state == "RAIN") return "ДОЩ";
      if (state == "SNOW") return "СНІГ";
      return "ХМАРНО";
    }
    if (state == "SUN") return "CLEAR";
    if (state == "RAIN") return "RAIN";
    if (state == "SNOW") return "SNOW";
    return "CLOUDY";
  }

  void drawWeatherTemperature(int value) {
    const String digits = String(value);
    const int16_t width = segmentStringWidth(digits, SEGMENT_LARGE);
    drawSegmentString(digits, 130 - width, 29, SEGMENT_LARGE,
                      TftUiTheme::FG);
    _gfx.drawRect(133, 30, 4, 4, TftUiTheme::ORANGE);
    drawLargeC(142, 36, TftUiTheme::ORANGE);
  }

  void drawVolumeFull(const Model &m) {
    _gfx.drawRect(3, 3, 154, 122, TftUiTheme::FG);
    _gfx.drawRect(5, 5, 150, 118, TftUiTheme::DIM);
    centerText(26, m.ukrainian ? "ГУЧНІСТЬ" : "Volume",
               m.ukrainian ? u8g2_font_6x12_t_cyrillic
                           : u8g2_font_7x14B_tf,
               TftUiTheme::FG);
    _gfx.drawFastHLine(13, 31, 134, TftUiTheme::CYAN);
    drawVolumeValue(m.volumePercent);
  }

  void drawVolumeValue(int percent) {
    const String digits = String(constrain(percent, 0, 100));
    const int16_t digitsW = segmentStringWidth(digits, SEGMENT_LARGE);
    font(u8g2_font_7x14B_tf, TftUiTheme::FG);
    const int16_t percentW = _text.getUTF8Width("%");
    const int16_t totalW = digitsW + 5 + percentW;
    const int16_t startX = (160 - totalW) / 2;
    drawSegmentString(digits, startX, 48, SEGMENT_LARGE, TftUiTheme::FG);
    textAt(startX + digitsW + 5, 78, "%", u8g2_font_7x14B_tf,
           TftUiTheme::FG);
  }

  void updateVolumeDirty(const Model &m) {
    if (!_cacheValid || m.volumePercent == _cache.volumePercent) return;
    clearRect(25, 42, 110, 43);
    drawVolumeValue(m.volumePercent);
  }

  void drawSleepFull(const Model &m) {
    // Keep the user-approved kitty asset byte-for-byte; animate only the Z area.
    TftUiAssets::drawApprovedKitty(_gfx);
    drawSleepZ(m);
  }

  void updateSleepDirty(const Model &m) {
    if (!_cacheValid) return;
    const uint8_t oldFrame = (_cache.nowMs / 450UL) % 4;
    const uint8_t newFrame = (m.nowMs / 450UL) % 4;
    if (oldFrame == newFrame) return;
    // Never clear into the approved kitty bitmap (it starts at y=53).
    // The small Z stays static; only medium/large Z symbols animate here.
    clearRect(108, 8, 52, 40);
    if (newFrame >= 1) drawZ(110, 34, 2, TftUiTheme::FG);
    if (newFrame >= 2) drawZ(130, 13, 3, TftUiTheme::FG);
  }

  void drawSleepZ(const Model &m) {
    const uint8_t frame = (m.nowMs / 450UL) % 4;
    drawZ(95, 49, 1, TftUiTheme::DIM);
    if (frame >= 1) drawZ(110, 34, 2, TftUiTheme::FG);
    if (frame >= 2) drawZ(130, 13, 3, TftUiTheme::FG);
  }

  void drawSleepKitty(int16_t x, int16_t y) {
    _gfx.fillRoundRect(x + 50, y + 22, 78, 45, 20, TftUiTheme::KITTY_DARK);
    _gfx.fillCircle(x + 104, y + 44, 23, TftUiTheme::KITTY_DARK);
    _gfx.fillCircle(x + 103, y + 44, 13, TftUiTheme::BG);
    _gfx.fillRoundRect(x + 10, y + 10, 66, 48, 16, TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 13, y + 16, x + 16, y - 2, x + 32, y + 13, TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 56, y + 11, x + 69, y - 5, x + 72, y + 21, TftUiTheme::KITTY_FUR);
    _gfx.fillTriangle(x + 17, y + 10, x + 18, y + 2, x + 27, y + 12, TftUiTheme::KITTY_PINK);
    _gfx.fillTriangle(x + 59, y + 8, x + 67, y - 1, x + 68, y + 15, TftUiTheme::KITTY_PINK);
    _gfx.fillRoundRect(x + 25, y + 48, 22, 14, 7, TftUiTheme::KITTY_FUR);
    _gfx.fillRoundRect(x + 43, y + 48, 23, 14, 7, TftUiTheme::KITTY_FUR);
    _gfx.drawLine(x + 26, y + 31, x + 31, y + 34, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 31, y + 34, x + 36, y + 31, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 50, y + 31, x + 55, y + 34, TftUiTheme::KITTY_DARK);
    _gfx.drawLine(x + 55, y + 34, x + 60, y + 31, TftUiTheme::KITTY_DARK);
    _gfx.fillTriangle(x + 41, y + 38, x + 47, y + 38, x + 44, y + 42, TftUiTheme::KITTY_PINK);
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
    for (uint8_t i = 0; i < 4; ++i)
      _gfx.fillRect(x + (3 - i) * scale, y + (i + 1) * scale, scale, scale, color);
  }

  void drawBtIcon(int16_t x, int16_t y, bool connected) {
    const uint16_t c = connected ? TftUiTheme::BLUE : TftUiTheme::DIM;
    _gfx.drawFastVLine(x + 3, y, 11, c);
    _gfx.drawLine(x + 3, y, x + 7, y + 3, c);
    _gfx.drawLine(x + 7, y + 3, x + 1, y + 8, c);
    _gfx.drawLine(x + 3, y + 10, x + 7, y + 7, c);
    _gfx.drawLine(x + 7, y + 7, x + 1, y + 2, c);
    if (!connected) _gfx.drawLine(x, y, x + 8, y + 10, TftUiTheme::RED);
  }

  void drawWifiIcon(int16_t x, int16_t y, bool connected) {
    const uint16_t c = connected ? TftUiTheme::CYAN : TftUiTheme::DIM;
    _gfx.drawCircle(x + 6, y + 9, 6, c);
    _gfx.drawCircle(x + 6, y + 9, 4, c);
    _gfx.drawCircle(x + 6, y + 9, 2, c);
    _gfx.fillRect(x - 1, y + 9, 15, 7, TftUiTheme::PANEL_BG);
    _gfx.fillCircle(x + 6, y + 9, 1, c);
    if (!connected) _gfx.drawLine(x, y + 2, x + 12, y + 11, TftUiTheme::RED);
  }

  void drawBatteryStatus(int16_t x, int16_t y, const Model &m) {
    constexpr int16_t bodyW = 13;
    constexpr int16_t bodyH = 8;
    const bool low = m.batteryPresent && !m.batteryCharging && m.batteryPercent <= 15;
    const uint16_t outline = (!m.batteryPresent || low)
      ? TftUiTheme::RED : TftUiTheme::FG;
    _gfx.drawRect(x, y, bodyW, bodyH, outline);
    _gfx.fillRect(x - 2, y + 2, 2, 4, outline);
    if (m.batteryPresent) {
      if (!low) {
        const int iconPercent = constrain(m.batteryIconPercent, 0, 100);
        const int fill = map(iconPercent, 0, 100, 0, 9);
        if (fill > 0) {
          _gfx.fillRect(x + bodyW - 2 - fill, y + 2, fill, 4,
                        TftUiTheme::batteryColor(iconPercent));
        }
      }
      if (low) {
        _gfx.drawLine(x + 4, y + 1, x + 7, y + 4, TftUiTheme::RED);
        _gfx.drawLine(x + 7, y + 4, x + 4, y + 4, TftUiTheme::RED);
        _gfx.drawLine(x + 4, y + 4, x + 6, y + 7, TftUiTheme::RED);
      }
    } else {
      _gfx.drawLine(x + 13, y + 8, x - 1, y - 1, TftUiTheme::RED);
      _gfx.drawLine(x + 12, y + 8, x - 2, y - 1, TftUiTheme::RED);
    }
    if (m.batteryPresent && m.batteryCharging) {
      drawChargingPlug(x + 15, y);
    }
    const String percent = m.batteryPresent
      ? String(constrain(m.batteryPercent, 0, 100)) + "%" : "--%";
    font(u8g2_font_5x8_tf, (!m.batteryPresent || low)
                           ? TftUiTheme::RED : TftUiTheme::FG);
    _text.setCursor(x + 22, 12);
    _text.print(percent);
  }

  void drawChargingPlug(int16_t x, int16_t y) {
    const uint16_t c = TftUiTheme::YELLOW;
    _gfx.drawFastVLine(x + 1, y, 2, c);
    _gfx.drawFastVLine(x + 3, y, 2, c);
    _gfx.drawFastHLine(x, y + 2, 5, c);
    _gfx.drawPixel(x, y + 3, c);
    _gfx.drawPixel(x + 4, y + 3, c);
    _gfx.drawFastHLine(x, y + 4, 5, c);
    _gfx.drawFastVLine(x + 2, y + 5, 3, c);
  }

  void drawWeatherIcon(int16_t x, int16_t y, const String &state, bool night) {
    if (state == "SUN") {
      if (night) drawMoon(x, y);
      else drawSun(x, y);
    } else if (state == "RAIN") {
      drawCloud(x, y);
      drawRainDrop(x + 11, y + 32);
      drawRainDrop(x + 23, y + 34);
      drawRainDrop(x + 35, y + 32);
    } else if (state == "SNOW") {
      drawCloud(x, y);
      drawSnowflake(x + 11, y + 35);
      drawSnowflake(x + 23, y + 38);
      drawSnowflake(x + 35, y + 35);
    } else {
      drawCloud(x, y);
    }
  }

  void drawRainDrop(int16_t x, int16_t y) {
    _gfx.drawPixel(x, y, TftUiTheme::CYAN);
    _gfx.drawFastHLine(x - 1, y + 1, 3, TftUiTheme::CYAN);
    _gfx.drawFastHLine(x - 1, y + 2, 3, TftUiTheme::CYAN);
    _gfx.drawFastHLine(x - 2, y + 3, 5, TftUiTheme::BLUE);
    _gfx.drawFastHLine(x - 2, y + 4, 5, TftUiTheme::BLUE);
    _gfx.drawPixel(x - 1, y + 3, TftUiTheme::CYAN);
    _gfx.drawFastHLine(x - 1, y + 5, 3, TftUiTheme::BLUE);
  }

  void drawSnowflake(int16_t x, int16_t y) {
    _gfx.drawFastHLine(x - 3, y, 7, TftUiTheme::FG);
    _gfx.drawFastVLine(x, y - 3, 7, TftUiTheme::FG);
    _gfx.drawLine(x - 2, y - 2, x + 2, y + 2, TftUiTheme::CYAN);
    _gfx.drawLine(x + 2, y - 2, x - 2, y + 2, TftUiTheme::CYAN);
    _gfx.drawPixel(x - 3, y - 1, TftUiTheme::BLUE);
    _gfx.drawPixel(x - 3, y + 1, TftUiTheme::BLUE);
    _gfx.drawPixel(x + 3, y - 1, TftUiTheme::BLUE);
    _gfx.drawPixel(x + 3, y + 1, TftUiTheme::BLUE);
    _gfx.drawPixel(x - 1, y - 3, TftUiTheme::BLUE);
    _gfx.drawPixel(x + 1, y - 3, TftUiTheme::BLUE);
    _gfx.drawPixel(x - 1, y + 3, TftUiTheme::BLUE);
    _gfx.drawPixel(x + 1, y + 3, TftUiTheme::BLUE);
  }

  uint16_t gradientColorForY(int16_t localY, int16_t diameter,
                             uint16_t top, uint16_t middle,
                             uint16_t bottom) {
    if (localY < diameter / 3) return top;
    if (localY < (diameter * 2) / 3) return middle;
    return bottom;
  }

  void fillGradientCircle(int16_t cx, int16_t cy, int16_t r,
                          uint16_t top, uint16_t middle,
                          uint16_t bottom) {
    for (int16_t dy = -r; dy <= r; ++dy) {
      int32_t rr = (int32_t)r * r - (int32_t)dy * dy;
      if (rr < 0) rr = 0;
      const int16_t dx = (int16_t)sqrt((double)rr);
      const uint16_t c = gradientColorForY(dy + r, r * 2 + 1,
                                           top, middle, bottom);
      _gfx.drawFastHLine(cx - dx, cy + dy, dx * 2 + 1, c);
    }
  }

  void fillBackgroundCircle(int16_t cx, int16_t cy, int16_t r) {
    for (int16_t dy = -r; dy <= r; ++dy) {
      int32_t rr = (int32_t)r * r - (int32_t)dy * dy;
      if (rr < 0) rr = 0;
      const int16_t dx = (int16_t)sqrt((double)rr);
      _gfx.drawFastHLine(cx - dx, cy + dy, dx * 2 + 1,
                         TftUiTheme::backgroundColorForY(cy + dy));
    }
  }

  void drawMoon(int16_t x, int16_t y) {
    fillGradientCircle(x + 19, y + 16, 15,
                       TftUiTheme::CLOUD_HI,
                       TftUiTheme::BLUE,
                       0x72BC);
    fillBackgroundCircle(x + 27, y + 9, 15);
    _gfx.fillRect(x + 37, y + 6, 2, 2, TftUiTheme::CYAN);
    _gfx.fillRect(x + 41, y + 16, 2, 2, TftUiTheme::CYAN);
    _gfx.fillRect(x + 35, y + 27, 2, 2, TftUiTheme::LIME);
  }

  void drawSun(int16_t x, int16_t y) {
    const uint16_t ray = TftUiTheme::ORANGE;
    fillGradientCircle(x + 20, y + 16, 11,
                       TftUiTheme::FG,
                       TftUiTheme::YELLOW,
                       TftUiTheme::ORANGE);
    _gfx.drawFastVLine(x + 20, y, 4, ray);
    _gfx.drawFastVLine(x + 20, y + 29, 4, ray);
    _gfx.drawFastHLine(x + 2, y + 16, 5, ray);
    _gfx.drawFastHLine(x + 34, y + 16, 5, ray);
    _gfx.drawLine(x + 7, y + 4, x + 10, y + 7, ray);
    _gfx.drawLine(x + 30, y + 25, x + 33, y + 28, ray);
    _gfx.drawLine(x + 33, y + 4, x + 30, y + 7, ray);
    _gfx.drawLine(x + 10, y + 25, x + 7, y + 28, ray);
  }

  void drawCloud(int16_t x, int16_t y) {
    _gfx.fillCircle(x + 11, y + 20, 8, TftUiTheme::CLOUD_LOW);
    _gfx.fillCircle(x + 23, y + 15, 11, TftUiTheme::CLOUD_LOW);
    _gfx.fillCircle(x + 35, y + 21, 8, TftUiTheme::CLOUD_LOW);
    _gfx.fillRoundRect(x + 4, y + 19, 40, 12, 5, TftUiTheme::CLOUD_LOW);

    _gfx.fillCircle(x + 12, y + 18, 6, TftUiTheme::CLOUD_MID);
    _gfx.fillCircle(x + 23, y + 14, 9, TftUiTheme::CLOUD_MID);
    _gfx.fillCircle(x + 34, y + 19, 6, TftUiTheme::CLOUD_MID);
    _gfx.fillRoundRect(x + 7, y + 17, 33, 9, 4, TftUiTheme::CLOUD_MID);

    _gfx.fillCircle(x + 20, y + 11, 5, TftUiTheme::CLOUD_HI);
    _gfx.fillRoundRect(x + 10, y + 15, 22, 4, 2, TftUiTheme::CLOUD_HI);
  }
};

} // namespace TftUi
