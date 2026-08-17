
## v4.0-sister-007-adkey-ux

- Debounced HU-055 ADKEY button handling.
- Widened calibrated ADKEY windows for BTN1/BTN2/BTN3.
- Made BTN2 setup hold tolerant to short ADC dropouts.
- Added runtime BTN2 long hold to open OLEG-SETUP.
- Changed runtime BTN1 long hold from instant language toggle to safe language menu: release first, then press BTN1=EN or BTN3=UA.

# Changelog

## 4.0-009 — SW1 GPIO27 deep sleep

- Added GPIO27 sensing of the switched MH-M18 VCC rail through a 91k/120k divider.
- A stable SW1 OFF level for 500 ms enables SSD1309 power-save and puts ESP32 into deep sleep.
- SW1 ON wakes ESP32 through RTC GPIO27 and performs a normal boot.
- Booting while SW1 is already OFF skips OLED, Wi-Fi, Bluetooth and I2S startup.

## structured-split-0.1

- Split monolithic `.ino` into ordered `src/*.h` modules.
- Preserved original behavior and code order.
- Added README, pinout notes, .gitignore, and original archive.
- Kept startup jingle as disabled/legacy until AudioTools rewrite.

## Next planned work

- Validate LoLin build from structured project.
- Stabilize AudioTools I2S output to MAX98357A.
- Re-check volume/clipping and I2S format.
- Later: rewrite startup jingle through AudioTools, not legacy `i2s_write()`.
