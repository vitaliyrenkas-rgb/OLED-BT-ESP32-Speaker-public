# Notes

## AudioTools migration

The LoLin build uses newer `ESP32-A2DP` and `AudioTools`.
The old `set_pin_config()` API is no longer available in ESP32-A2DP 1.8.10.

Current direction:
- `I2SStream i2s;`
- `BluetoothA2DPSink a2dp_sink(i2s);`
- configure pins with `i2s.defaultConfig(TX_MODE)`

## Startup jingle

The old jingle implementation used legacy ESP-IDF I2S calls:
- `i2s_write`
- `i2s_config_t`
- `i2s_driver_install`

That does not compile in the current AudioTools/Core 3.3.8 build unless legacy I2S is explicitly restored.
Keep jingle disabled until rewritten via AudioTools.
