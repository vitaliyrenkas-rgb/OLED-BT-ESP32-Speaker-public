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
