// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

   // HU-055 USA Gift Build:
  // TEMPORARY: SW1/deep-sleep handling disabled for transparent OLED bring-up.
  // setupPowerSwitchSense();
  // handlePowerSwitchAtBoot();
  // Serial.printf("OLEG 4 Sister: SW1 sense on GPIO%d, level=%d\n",
  //               SW1_SENSE_PIN, digitalRead(SW1_SENSE_PIN));

  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(USB_VBUS_ADC_PIN, ADC_11db);
  if (VOLUME_POT_ENABLED && VOLUME_ADC_PIN >= 0) {
    analogSetPinAttenuation(VOLUME_ADC_PIN, ADC_11db);
  }
  analogSetPinAttenuation(ADKEY_ADC_PIN, ADC_11db);
  updateBattery();

  u8g2.begin();
  u8g2.enableUTF8Print();

  setupButtons();
  Serial.println("OLEG 4 Sister: ADKEY buttons on GPIO35");
  Serial.println("Runtime: hold BTN1 for 7s to toggle language");
  Serial.println("Boot: hold BTN2 for 7s for OLEG-SETUP");
  loadOrSelectLanguage();
  loadSpeakerConfig();

  if (configPortalRequestedAtBoot()) {
    startConfigPortal("BTN2 boot hold");
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

auto cfg = i2s.defaultConfig(TX_MODE);
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
  a2dp_sink.set_stream_reader(read_data_stream, true);
  a2dp_sink.set_avrc_rn_playstatus_callback(playback_status_callback);

  // Keep stable v3.5 audio baseline for the first HU-055/PCM5102A target patch.
  a2dp_sink.set_mono_downmix(true);

  // v3.5: raise initial A2DP digital volume after hardware GND fix.
  // Conservative level; 127 caused bass/contact swings on breadboard.
  a2dp_sink.set_volume(100);

  a2dp_sink.start(speakerConfig.btDeviceName.c_str());

  requestRedraw();
}
