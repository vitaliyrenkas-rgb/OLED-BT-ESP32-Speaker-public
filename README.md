# OLEG Bluetooth Speaker LoLin

Structured Arduino project for OLEG Bluetooth speaker firmware.

This project was split from the monolithic sketch:

`Vitalik_Speaker_HARD_v3_2jingle_ui_cosmetic_LoLin32_MicroPython.ino`

## Current purpose

- Keep the latest LoLin / AudioTools migration sketch in Git-friendly form.
- Preserve the original behavior while making the project easier to patch.
- Continue work on LoLin + MAX98357A audio, OLED UI, metadata, weather, and config flow.

## Build target

Arduino IDE / ESP32 classic target:

- Board: `ESP32 Dev Module`
- ESP32 Arduino core seen in logs: `3.3.8`
- ESP32-A2DP seen in logs: `1.8.10`
- AudioTools seen in logs: `1.2.4`

## Structure

- `OLEG_BT_Speaker_LoLin.ino` — root entry point with includes only.
- `src/00_config.h` — Wi-Fi/weather/user config.
- `src/01_display_pins.h` — OLED setup.
- `src/02_audio_pins.h` — MAX98357A/I2S pins.
- `src/03_button_pins.h` — button GPIO definitions.
- `src/04_power_switch.h` — SW1 GPIO27 sensing, OLED power-save, deep sleep and wakeup.
- `src/05_state.h` — global state and objects.
- `src/07_config_language.h` — Preferences/language selection.
- `src/09_wifi_weather.h` — Wi-Fi, NTP, weather.
- `src/10_icons.h` / `src/11_common_ui.h` / `src/15_screens.h` — UI rendering.
- `src/12_playback_pcm.h` / `src/13_track_time.h` / `src/14_player_helpers.h` — playback/EQ/timer helpers.
- `src/17_callbacks.h` — AVRCP callbacks.
- `src/19_config_reset.h` — config/language reset combo.
- `src/20_startup_jingle_disabled.h` — startup jingle block, currently disabled/legacy.
- `archive/original/` — original monolithic sketch backup.
- `docs/SW1_GPIO27_TEST.md` — wiring, expected voltages and bench smoke test.

## Notes

The project is intentionally split as ordered headers included by the main `.ino`.
This preserves the original single-translation-unit Arduino behavior and avoids a risky `.cpp` refactor before the LoLin audio migration is stable.

## Known active issue

The old startup jingle used legacy `driver/i2s.h` / `i2s_write()`.
Under ESP32 core 3.3.8 + ESP32-A2DP 1.8.10 + AudioTools, that legacy jingle must remain disabled until rewritten through AudioTools.

## v4.0-sister-009-sw1-deep-sleep

- `GPIO27` reads the switched MH-M18 `VCC` point through a `91k/120k` divider.
- Stable `SW1 OFF` for 500 ms turns the SSD1309 display off and enters ESP32 deep sleep.
- `SW1 ON` wakes the ESP32 on RTC GPIO27 HIGH and starts normally.
