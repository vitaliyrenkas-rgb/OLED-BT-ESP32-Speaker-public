// Auto-split from monolithic OLEG sketch.
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
  // OLEG v3.5-005:
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
// API: a2dp_sink.set_stream_reader(read_data_stream, true);
// Data is normally 44.1kHz, stereo, 16-bit PCM.
void read_data_stream(const uint8_t *data, uint32_t length) {
  static uint32_t lastLogMs = 0;
  static uint32_t bytesAccum = 0;
  static uint32_t callsAccum = 0;
  static uint32_t peakAccum = 0;
  static uint32_t avgAccum = 0;
  static uint32_t avgCount = 0;

  const int16_t *samples = (const int16_t*)data;
  uint32_t sampleCount = length / 2;

  uint32_t sumAbs = 0;
  uint32_t used = 0;
  uint32_t peak = 0;

  // Keep callback light: sample only every 8th int16.
  for (uint32_t i = 0; i < sampleCount; i += 8) {
    int32_t s = samples[i];
    if (s < 0) s = -s;

    uint32_t a = (uint32_t)s;
    sumAbs += a;
    if (a > peak) peak = a;
    used++;
  }

  if (used == 0) return;

  uint16_t avg = (uint16_t)(sumAbs / used);
  pcmLevelRaw = avg;

  if (avg > 250) {
    lastPcmAudioMs = millis();
  }

  bytesAccum += length;
  callsAccum++;
  avgAccum += avg;
  avgCount++;
  if (peak > peakAccum) peakAccum = peak;

  uint32_t now = millis();
  if (now - lastLogMs >= 1000) {
    uint32_t meanAvg = avgCount ? (avgAccum / avgCount) : 0;

    Serial.print("[PCM] calls=");
    Serial.print(callsAccum);
    Serial.print(" bytes=");
    Serial.print(bytesAccum);
    Serial.print(" avg=");
    Serial.print(meanAvg);
    Serial.print(" peak=");
    Serial.print(peakAccum);
    Serial.print(" last=");
    Serial.println(avg);

    lastLogMs = now;
    bytesAccum = 0;
    callsAccum = 0;
    peakAccum = 0;
    avgAccum = 0;
    avgCount = 0;
  }
}

void playback_status_callback(esp_avrc_playback_stat_t playback) {
  // FIX v2.10: pause/stop freezes both timer and EQ.
  if (playback == ESP_AVRC_PLAYBACK_PLAYING) setPlaybackActive(true);
  else setPlaybackActive(false);
}