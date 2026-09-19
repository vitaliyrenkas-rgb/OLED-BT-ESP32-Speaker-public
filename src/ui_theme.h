#pragma once

#include <Arduino.h>

namespace TftUiTheme {

constexpr int16_t WIDTH  = 160;
constexpr int16_t HEIGHT = 128;

constexpr int16_t TOPBAR_BOTTOM = 18;
constexpr int16_t NAVBAR_TOP    = 107;

// RT-003 v5.0: Winamp-inspired RGB565 palette. Black remains dominant;
// the body receives only a very subtle black-to-deep-blue vertical tint.
constexpr uint16_t BG             = 0x0000;
constexpr uint16_t PANEL_BG       = 0x0021;
constexpr uint16_t BODY_BG_BOTTOM = 0x0042;
constexpr uint16_t SEGMENT_OFF    = 0x1105;
constexpr uint16_t FG             = 0xFFDF; // warm white
constexpr uint16_t DIM            = 0x9CF3; // soft grey
constexpr uint16_t LINE           = 0x18C7; // restrained blue-grey
constexpr uint16_t CYAN       = 0x07FF;
constexpr uint16_t BLUE       = 0x359F;
constexpr uint16_t GREEN      = 0x07E0;
constexpr uint16_t LIME       = 0xAFE5;
constexpr uint16_t YELLOW     = 0xFFE0;
constexpr uint16_t RED        = 0xF800;
constexpr uint16_t ORANGE     = 0xFD20;
constexpr uint16_t CLOUD_HI   = 0xBF7F;
constexpr uint16_t CLOUD_MID  = 0x565D;
constexpr uint16_t CLOUD_LOW  = 0x3478;
constexpr uint16_t CLOUD      = CLOUD_MID;
constexpr uint16_t KITTY_FUR  = 0xE71C;
constexpr uint16_t KITTY_DARK = 0x7BEF;
constexpr uint16_t KITTY_PINK = 0xFBAE;
constexpr uint16_t NAV_IDLE_TOP      = 0x10A5;
constexpr uint16_t NAV_IDLE_BOTTOM   = 0x0021;
constexpr uint16_t NAV_ACTIVE_TOP    = 0x5EFF;
constexpr uint16_t NAV_ACTIVE_BOTTOM = 0x1B7B;
constexpr uint16_t NAV_ACTIVE_BORDER = 0x8FFF;

inline uint16_t batteryColor(int percent) {
  if (percent <= 15) return RED;
  if (percent <= 35) return ORANGE;
  return LIME;
}

inline uint16_t backgroundColorForY(int16_t y) {
  if (y < TOPBAR_BOTTOM || y > NAVBAR_TOP) return PANEL_BG;

  const int16_t span = NAVBAR_TOP - TOPBAR_BOTTOM;
  const int16_t pos = constrain(y - TOPBAR_BOTTOM, 0, span);
  const uint8_t endG = (BODY_BG_BOTTOM >> 5) & 0x3F;
  const uint8_t endB = BODY_BG_BOTTOM & 0x1F;
  const uint8_t g = (uint16_t)endG * pos / span;
  const uint8_t b = (uint16_t)endB * pos / span;
  return ((uint16_t)g << 5) | b;
}

inline uint16_t blend565(uint16_t from, uint16_t to,
                         uint8_t step, uint8_t steps) {
  if (steps == 0 || step >= steps) return to;

  const int16_t fromR = (from >> 11) & 0x1F;
  const int16_t fromG = (from >> 5) & 0x3F;
  const int16_t fromB = from & 0x1F;
  const int16_t toR = (to >> 11) & 0x1F;
  const int16_t toG = (to >> 5) & 0x3F;
  const int16_t toB = to & 0x1F;

  const uint16_t r = fromR + (toR - fromR) * step / steps;
  const uint16_t g = fromG + (toG - fromG) * step / steps;
  const uint16_t b = fromB + (toB - fromB) * step / steps;
  return (r << 11) | (g << 5) | b;
}

inline uint16_t eqColorForRow(int rowFromBottom, int maxRows) {
  static const uint16_t DISCO_GRADIENT[12] = {
    0xFEC9, 0xDF08, 0x86E9, 0x3E8C,
    0x2652, 0x2618, 0x2D7D, 0x3C1E,
    0x72BC, 0xBA19, 0xE9EF, 0xF9C9
  };
  if (maxRows <= 1) return DISCO_GRADIENT[0];
  const int row = constrain(rowFromBottom, 0, maxRows - 1);
  const int index = row * 11 / (maxRows - 1);
  return DISCO_GRADIENT[index];
}

} // namespace TftUiTheme
