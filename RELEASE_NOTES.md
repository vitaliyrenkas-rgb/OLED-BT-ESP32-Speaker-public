# Release Notes — OLED BT Speaker

## v3.5.0 public release candidate

This release line turns the original OLED Bluetooth speaker prototype into a usable real-device firmware with local configuration, stable audio, battery/charging telemetry, volume control, and a sleep screen.

### Highlights

- Stable Bluetooth A2DP audio baseline for ESP32 + MAX98357A.
- Player / Clock / Weather OLED screens.
- Local AP configuration portal: `OLED-SETUP` at `http://192.168.4.1`.
- Runtime config stored in NVS through `speakerConfig`.
- Dummy public-safe defaults in `src/00_config.h`.
- Battery voltage calibration for the reference 1S Li-ion build.
- Smoothed physical volume potentiometer.
- Sleeping kitty OLED idle screen with animated Z symbols.

### Configuration portal

Default credentials:

- Username: `BTAdmin`
- Password: `BTPassword`

The default password is public and should be changed during first setup for any real deployment.

### Hardware reference

- ESP32 / LoLin-style board.
- MAX98357A I2S amplifier.
- 128×64 I2C OLED display.
- Battery ADC on GPIO34.
- USB/VBUS sense on GPIO33.
- Volume potentiometer on GPIO35.

### Privacy cleanup for public release

The public tree uses placeholder Wi-Fi credentials and sanitized default strings. Old personal names, local absolute paths, and private Wi-Fi details were removed or replaced in the current release files.

### Known limitations

- No OTA/SD boot picker in this release.
- Factory-reset confirmation flow is planned for a later minor release.
- OpenWeather API key must be supplied by the user through the portal.
- Enclosed-build programming/data access depends on the physical build.

## Development milestones included

- Config storage layer.
- Neutral config naming.
- Runtime config wiring.
- AP/local configuration portal.
- Battery and volume-pot polish.
- Sleep kitty screen.
