# OLEG v3.5 Scope

## Current baseline

- Branch baseline: feature/v3.5-audio-breadboard-baseline
- Audio status: provisional breadboard baseline
- Audio config:
  - AudioTools defaultConfig()
  - mono_downmix(true)
  - set_volume(100)
- Hardware note:
  - common GND fixed
  - volume waves may still be breadboard/contact related
  - do not chase audio dips in code until final wiring is cleaner

## v3.5 Goals

### 1. UI alignment / polish

- Align topbar elements:
  - Bluetooth icon
  - Wi-Fi icon
  - centered time
  - weather/temp/battery area
- Check Player screen:
  - timer position
  - metadata text
  - marquee area:
  - Bluetooth icon
  - Wi-Fi icon
  - centered time
  -
  - EQ blocks
- Check Weather screen:
  - icon placement
  - temperature
  - feels-like text
  - weather label
- Check Clock screen:
  - time/date/day layout
- Check Language screen:
  - En / v3.5 / UA bottom line alignment

### 2. Wi-Fi / config portal manager

- Add/verify Wi-Fi portal manager flow.
- Keep BT audio safe:
  - no aggressive Wi-Fi reconnect while BT is active
  - portal only on explicit config/reset flow
- Preserve current language/config reset behavior.

### 3. Weather behavior

- Keep weather refresh interval: 1 hour.
- Refresh only when !btConnected.
- Preserve startup sync behavior.
- Later optional: manual refresh action/menu, if low-risk.

### 4. Audio freeze

Do not change audio code in v3.5 unless a new repeated symptom is confirmed on stable wiring.

Frozen for now:
- no NoVolumeControl
- no set_volume(127)
- no I2S format experiments
- no queued sink
- no raw stream reader
- no legacy rollback

### 5. QA checklist

- Build/upload on LoLin.
- BT pairing/connect/reconnect.
- Playback starts.
- Pause/resume.
- Track timer works.
- Metadata works, including Cyrillic.
- Player/Clock/Weather buttons work.
- Language reset combo works.
- Weather display works.
- Battery icon behavior acceptable for current ADC wiring.
- No OLED layout regressions.
- No ESP restart during normal playback.

## Out of scope for v3.5

- Startup jingle rewrite.
- Final MAX98357A gain hardware tuning.
- Battery divider calibration.
- Deep sync-after-restart investigation.
- Perfect audio stability on breadboard wiring.
