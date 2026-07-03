// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(USB_VBUS_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(VOLUME_ADC_PIN, ADC_11db);
  updateBattery();

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  u8g2.enableUTF8Print();

  setupButtons();
  // FIX v2.13: No external reset pin. Config reset is BTN_PLAYER + BTN_WEATHER hold 5s.
  Serial.println("Config reset: hold BTN_PLAYER + BTN_WEATHER for 5 seconds");
  Serial.println("Config portal: hold BTN_CLOCK during boot for OLED-SETUP");
  loadOrSelectLanguage();
  loadSpeakerConfig();

  if (configPortalRequestedAtBoot()) {
    startConfigPortal("BTN_CLOCK boot hold");
    return;
  }

  currentScreen = SCREEN_GREETING;
  greetingUntil = millis() + 2500;
  lastUserInteractionMs = millis();
  drawUI();

  // Startup sync: Wi-Fi -> NTP/weather -> Wi-Fi OFF.
  // Bluetooth starts only after Wi-Fi is off.
  updateWeatherCycle();

  // FIX v3.2: play short startup jingle before A2DP owns I2S.
  // playStartupJingle();

auto cfg = i2s.defaultConfig();
cfg.pin_bck = I2S_BCLK;
cfg.pin_ws = I2S_LRC;
cfg.pin_data = I2S_DOUT;
i2s.begin(cfg);

  a2dp_sink.set_avrc_connection_state_callback(avrc_connection_state_callback);
  a2dp_sink.set_avrc_metadata_attribute_mask(
    ESP_AVRC_MD_ATTR_TITLE |
    ESP_AVRC_MD_ATTR_ARTIST |
    ESP_AVRC_MD_ATTR_ALBUM |
    ESP_AVRC_MD_ATTR_PLAYING_TIME
  );
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  // FIX v2.10: real audio level from PCM stream + AVRCP playback status.
  a2dp_sink.set_stream_reader(read_data_stream);
  a2dp_sink.set_avrc_rn_playstatus_callback(playback_status_callback);

  // v3.3 test: downmix stereo A2DP to mono before I2S output.
  // Safe for mono MAX98357A speaker path; does not change I2S, callbacks, or stream reader.
  a2dp_sink.set_mono_downmix(true);

  // v3.5: raise initial A2DP digital volume after hardware GND fix.
  // Conservative level; 127 caused bass/contact swings on breadboard.
  a2dp_sink.set_volume(100);

  a2dp_sink.start(speakerConfig.btDeviceName.c_str());

  requestRedraw();
}
