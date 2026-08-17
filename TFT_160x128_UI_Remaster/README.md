# OLEG TFT 1.8" 160×128 — UI Remaster

Branch: `feature/tft-160x128-ui-remaster`

This folder is the source-of-truth for the approved TFT adaptation of the existing OLEG OLED UI.

## Approved design canon

This is a **remaster, not a redesign**.

Keep the original OLED composition and UI DNA:

- top bar: Bluetooth + Wi-Fi on the left, time centered, temperature and battery on the right;
- thin horizontal separator under the top bar;
- Player: large duration in the center, brick EQ on both sides, title and artist below, marquee behavior retained for integration;
- Clock: large time + date;
- Weather: weather icon left, temperature center, humidity right, small secondary labels;
- bottom separator + Player / Clock / Weather navigation, active item inverted;
- Volume overlay: same double-frame idea, remastered for the larger canvas;
- Sleep: same sleeping-kitty scene and animated Z symbols, but redrawn at higher detail/resolution;
- black remains the dominant background;
- color is used as restrained semantic accent, not as decoration.

## What changed versus the 128×64 OLED

- logical canvas: 160×128;
- larger, smoother U8g2 fonts rendered through `U8g2_for_Adafruit_GFX`;
- RGB565 accent colors;
- higher-resolution BT / Wi-Fi / battery / weather icons;
- larger brick EQ with color by height;
- procedural high-resolution sleep kitty so proportions/details can be tuned without regenerating a bitmap asset;
- extra vertical space is used for breathing room, not for giant tiles/panels.

## Files

- `ui_theme.h` — palette and fixed 160×128 geometry.
- `tft_ui_renderer.h` — hardware-independent renderer for Player / Clock / Weather / Volume / Sleep.
- `TFT_160x128_UI_Remaster.ino` — isolated visual bench sketch.

## Deliberately NOT defined

The following remain unknown until confirmed from current physical wiring/module evidence:

- TFT CS GPIO;
- TFT DC GPIO;
- TFT RST GPIO;
- TFT MOSI GPIO;
- TFT SCLK GPIO;
- exact `Adafruit_ST7735::initR()` profile;
- exact rotation value;
- logic voltage tolerance;
- backlight power/current topology.

The demo sketch therefore stops at compile time until the required `OLEG_TFT_*` macros are explicitly supplied.

## Libraries

- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library
- U8g2_for_Adafruit_GFX

`U8g2_for_Adafruit_GFX` is used intentionally so the TFT version can retain U8g2 font character and UTF-8/Unicode support instead of replacing the UI typography with the default Adafruit bitmap font.

## Integration rule

Do not rewrite the existing OLED firmware to test this UI.

First validate the isolated TFT renderer. After physical display PASS, add a thin adapter from the existing OLEG runtime state (`btConnected`, `batteryPercent`, `title`, `artist`, weather, time, volume, etc.) into `TftUi::Model`.

The original OLED behavior remains the behavioral reference. TFT work should be limited to rendering/layout unless a separate behavioral change is explicitly approved.

## Point-fix workflow

When changing appearance, prefer changing exactly one of:

1. coordinates / spacing in `tft_ui_renderer.h`;
2. palette in `ui_theme.h`;
3. font choice/size;
4. one icon function;
5. one screen function.

Avoid global rewrites after a screen is visually accepted.
