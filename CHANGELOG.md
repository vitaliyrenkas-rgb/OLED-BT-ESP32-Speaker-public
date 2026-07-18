
## v4.0-sister-007-adkey-ux

- Debounced HU-055 ADKEY button handling.
- Widened calibrated ADKEY windows for BTN1/BTN2/BTN3.
- Made BTN2 setup hold tolerant to short ADC dropouts.
- Added runtime BTN2 long hold to open OLEG-SETUP.
- Changed runtime BTN1 long hold from instant language toggle to safe language menu: release first, then press BTN1=EN or BTN3=UA.

# Changelog

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
