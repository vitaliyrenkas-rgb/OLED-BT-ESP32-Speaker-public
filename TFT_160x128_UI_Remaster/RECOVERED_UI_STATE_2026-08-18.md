# RECOVERED UI STATE — OLEG TFT 1.8" 160×128

**Recovered:** 2026-08-18
**Branch:** `feature/tft-160x128-ui-remaster`

This file exists because the fresh chat context was lost twice during the UI iteration. It is the continuation checkpoint for the approved TFT work.

## HARD SOURCE OF TRUTH

The **approved 160×128 mockups** are the visual source of truth.

This is NOT permission to fall back to the old OLED geometry.

The existing OLED firmware remains the behavioral/style ancestor, but the current TFT layout must follow the approved mockups, not a literal 128×64 coordinate upscale.

User-approved rule:

> approved mockup = hard source of truth; do not freely redraw, move blocks, add new UI, or reinterpret the composition; future changes are point fixes only.

## APPROVED TFT LAYOUT

### Common top bar

- Bluetooth icon left.
- Wi-Fi icon left next to Bluetooth.
- Time centered.
- Temperature toward the right.
- Battery icon + visible battery percentage at far right.
- Thin horizontal separator beneath the status bar.
- Black background dominates; color is semantic accent only.

### Player

Approved composition:

- Huge `02:36` duration centered in the main area.
- Tall colored brick EQ on BOTH sides of the duration.
- EQ color rises cyan → green/lime → yellow.
- `Metallica - One` is a large centered title below duration/EQ.
- `...And Justice for All` is a smaller dimmer centered artist/album line below.
- Bottom separator.
- `Player / Clock / Weather` navigation, with Player inverted/active.

Do NOT replace this with the old OLED geometry where duration/title/artist are tightly packed into the 64px layout.

### Clock

Approved composition:

- Huge `23:47` dominates the center.
- `SUN 17 AUG` centered underneath.
- Common top bar retained.
- Bottom nav retained, Clock inverted/active.

### Weather

Approved composition is three clear visual blocks:

- LEFT: large high-resolution colored weather icon; label such as `CLEAR` beneath.
- CENTER: large signed temperature such as `+21`; `Feels 19C` beneath.
- RIGHT: large humidity value such as `64%`; `HUMIDITY` beneath.
- Common top bar and bottom nav retained; Weather inverted/active.

Weather icon concept stays derived from the existing OLEG icon language, but at higher resolution and in color.

### Volume

Approved composition:

- Double thin frame around the screen.
- `Volume` centered near the top.
- One thin cyan horizontal separator beneath the title.
- Huge centered `37%`.
- NO extra bottom progress bar was approved.

### Sleep

- Same OLEG sleeping-kitty idea/scene, but higher resolution.
- Animated `Z` symbols remain.
- The kitty should remain recognizably the approved OLEG sleep screen concept, not a random unrelated cat redesign.

## CRITICAL REDRAW RULE — NO BLACK CURTAIN

The previous implementation mistake was clearing the whole display on every animation frame.

Approved renderer behavior:

- Full `fillScreen(BG)` is allowed only when entering/changing a scene.
- On the same scene, animated/state changes update only their dirty region.
- Player EQ redraws only EQ rectangles.
- Top bar status changes redraw only the affected icon/text rectangle.
- Volume changes redraw only the central numeric value region.
- Sleep animation redraws only the `Z` region.
- Weather body may redraw its body region when weather data/state changes, but not the whole screen.

A full-screen black clear on every frame is a regression and must not be reintroduced.

## ACTIVE RENDERER STRATEGY

Two renderer files intentionally coexist:

- `tft_ui_renderer.h` — older/incorrect experimental renderer, retained only as rollback/reference insurance.
- `tft_ui_renderer_approved.h` — ACTIVE renderer for the approved mockup iteration.

The bench sketch must include:

```cpp
#include "tft_ui_renderer_approved.h"
```

Do not silently switch back to `tft_ui_renderer.h`.

## CURRENT BENCH — CONFIRMED

```text
TFT SCK/SCL  -> Lolita GPIO18
TFT MOSI/SDA -> Lolita GPIO23
TFT CS       -> Lolita GPIO19
TFT DC/A0    -> Lolita GPIO22
TFT RST/RES  -> Lolita GPIO16
TFT GND      -> Lolita GND
TFT LED/BL   -> ESP32 3.3V
```

Current proven/test baseline used by the showroom sketch:

```cpp
tft.initR(INITR_BLACKTAB);
tft.setRotation(1);
tft.setSPISpeed(27000000UL);
```

Landscape rotation `1` is accepted for the test stand.

## CURRENT SHOWROOM

`TFT_160x128_UI_Remaster.ino` is the isolated visual bench.

Current sequence:

1. Player + animated brick EQ
2. Clock
3. Weather / clear day
4. Weather / clear night
5. Weather / cloud
6. Weather / rain
7. Weather / snow
8. Volume + animated value
9. Sleep kitty + animated Z

Scene duration is currently 30 seconds.

## POINT-FIX RULE

Once a screen is accepted, fix only the affected thing:

1. coordinate/spacing block;
2. one font choice/size;
3. one icon;
4. one palette value;
5. one screen function;
6. one dirty-redraw region.

Do NOT replace the whole renderer because one element is wrong.

## CONTINUATION MARKER

**Continue from `tft_ui_renderer_approved.h`.**

The immediate task is physical compile/upload/visual validation of the approved mockup renderer on the 160×128 ST7735 bench, followed by point fixes from actual screen photos.

Do not redesign the UI again before that validation.
