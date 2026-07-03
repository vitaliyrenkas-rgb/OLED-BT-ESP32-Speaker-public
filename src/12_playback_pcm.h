// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= PLAYBACK / PCM HELPERS =================
void startTrackTimer() {
  if (!trackTimerRunning) {
    trackStartedAtMs = millis();
    trackTimerRunning = true;
    lastPlaybackStateChangeMs = millis();
  }
}

void pauseTrackTimer() {
  if (trackTimerRunning) {
    trackElapsedOffsetMs += millis() - trackStartedAtMs;
    trackTimerRunning = false;
    lastPlaybackStateChangeMs = millis();
  }
}

void resetTrackTimer() {
  trackElapsedOffsetMs = 0;
  trackStartedAtMs = millis();
  trackTimerRunning = playbackActive;
}

void setPlaybackActive(bool active) {
  if (playbackActive == active) return;

  playbackActive = active;

  if (playbackActive) startTrackTimer();
  else pauseTrackTimer();

  requestRedraw();
}

uint8_t audioLevelToBricks() {
  // OLED v3.5-005:
  // Make Player brick EQ about 30% more responsive without touching audio output.
  uint32_t level = ((uint32_t)pcmLevelRaw * 14U) / 10U;
  if (level > 65535U) level = 65535U;

  if (!playbackActive || millis() - lastPcmAudioMs > 700) return 0;

  // FIX v2.11:
  // Increased sensitivity for more lively EQ movement.
  // FIX v3.0:
  // Slightly more sensitive EQ response.
  if (level < 120) return 0;
  if (level < 350) return 1;
  if (level < 800) return 2;
  if (level < 1600) return 3;
  if (level < 3200) return 4;
  if (level < 6000) return 5;
  return 6;
}

// PCM tap from ESP32-A2DP.
// API: a2dp_sink.set_stream_reader(read_data_stream);
// Data is normally 44.1kHz, stereo, 16-bit PCM.
void read_data_stream(const uint8_t *data, uint32_t length) {
  const int16_t *samples = (const int16_t*)data;
  uint32_t sampleCount = length / 2;

  uint32_t sumAbs = 0;
  uint32_t used = 0;

  // Keep callback light: sample only every 8th int16.
  for (uint32_t i = 0; i < sampleCount; i += 8) {
    int32_t s = samples[i];
    if (s < 0) s = -s;
    sumAbs += (uint32_t)s;
    used++;
  }

  if (used == 0) return;

  uint16_t avg = (uint16_t)(sumAbs / used);
  pcmLevelRaw = avg;

  if (avg > 250) lastPcmAudioMs = millis();
}

void playback_status_callback(esp_avrc_playback_stat_t playback) {
  // FIX v2.10: pause/stop freezes both timer and EQ.
  if (playback == ESP_AVRC_PLAYBACK_PLAYING) setPlaybackActive(true);
  else setPlaybackActive(false);
}
