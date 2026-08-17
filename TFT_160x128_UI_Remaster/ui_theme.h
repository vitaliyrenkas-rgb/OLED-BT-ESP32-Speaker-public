#pragma once

#include <Arduino.h>

namespace TftUiTheme {

constexpr int16_t WIDTH  = 160;
constexpr int16_t HEIGHT = 128;

constexpr int16_t TOPBAR_BOTTOM = 18;
constexpr int16_t NAVBAR_TOP    = 107;

// Small, restrained RGB565 palette. Black remains the dominant surface.
constexpr uint16_t BG         = 0x0000; // black
constexpr uint16_t FG         = 0xFFDF; // warm white
constexpr uint16_t DIM        = 0x9CF3; // soft grey
constexpr uint16_t LINE       = 0xC618; // light grey
constexpr uint16_t CYAN       = 0x07FF;
constexpr uint16_t BLUE       = 0x359F;
constexpr uint16_t GREEN      = 0x07E0;
constexpr uint16_t LIME       = 0xAFE5;
constexpr uint16_t YELLOW     = 0xFFE0;
constexpr uint16_t RED        = 0xF800;
constexpr uint16_t ORANGE     = 0xFD20;
constexpr uint16_t CLOUD      = 0xC618;
constexpr uint16_t KITTY_FUR  = 0xE71C;
constexpr uint16_t KITTY_DARK = 0x7BEF;
constexpr uint16_t KITTY_PINK = 0xFBAE;

inline uint16_t batteryColor(int percent) {
  if (percent <= 15) return RED;
  if (percent <= 35) return ORANGE;
  return LIME;
}

inline uint16_t eqColorForRow(int rowFromBottom, int maxRows) {
  if (maxRows <= 1) return CYAN;
  if (rowFromBottom >= maxRows - 2) return YELLOW;
  if (rowFromBottom >= maxRows / 2) return LIME;
  return CYAN;
}

} // namespace TftUiTheme
