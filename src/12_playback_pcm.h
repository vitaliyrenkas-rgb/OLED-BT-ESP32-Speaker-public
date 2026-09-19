// RT-003 v5.0 PCM/playback runtime.

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

void clearPcmEqBands() {
  for (uint8_t band = 0; band < 4; ++band) pcmEqBands[band] = 0;
}

void setPlaybackActive(bool active) {
  if (playbackActive == active) return;

  playbackActive = active;

  if (playbackActive) {
    startTrackTimer();
  } else {
    pauseTrackTimer();
    clearPcmEqBands();
  }

  requestRedraw();
}

uint8_t audioLevelToBricks() {
  // Inherited playback behavior:
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

  // Four real frequency envelopes.  These one-pole filters split the mono
  // PCM tap at roughly 220 Hz, 880 Hz and 3.4 kHz without an FFT or heap use.
  // The callback remains read-only: A2DP still forwards the original stereo
  // samples to I2S unchanged.
  static int32_t lowPass = 0;
  static int32_t midPass = 0;
  static int32_t highPass = 0;
  static uint32_t envelope[4] = {0, 0, 0, 0};
  static uint32_t sharedPeak = 900;

  const int16_t *samples = (const int16_t*)data;
  uint32_t sampleCount = length / 2;

  uint64_t sumAbs = 0;
  uint64_t bandSum[4] = {0, 0, 0, 0};
  uint32_t frames = 0;
  uint32_t peak = 0;

  for (uint32_t i = 0; i + 1 < sampleCount; i += 2) {
    const int32_t mono = ((int32_t)samples[i] + (int32_t)samples[i + 1]) / 2;

    lowPass += (mono - lowPass) / 32;
    midPass += (mono - midPass) / 8;
    highPass += ((mono - highPass) * 3) / 8;

    int32_t split[4] = {
      lowPass,
      midPass - lowPass,
      highPass - midPass,
      mono - highPass
    };

    int32_t monoAbs = mono;
    if (monoAbs < 0) monoAbs = -monoAbs;
    const uint32_t a = (uint32_t)monoAbs;
    sumAbs += a;
    if (a > peak) peak = a;

    for (uint8_t band = 0; band < 4; ++band) {
      if (split[band] < 0) split[band] = -split[band];
      bandSum[band] += (uint32_t)split[band];
    }
    frames++;
  }

  if (frames == 0) return;

  const uint16_t avg = (uint16_t)(sumAbs / frames);
  pcmLevelRaw = avg;

  if (avg > 250) {
    lastPcmAudioMs = millis();
  }

  // Compensate for unequal band widths, then apply fast attack / slow release.
  static const uint8_t BAND_GAIN[4] = {3, 2, 2, 1};
  uint32_t loudest = 0;
  for (uint8_t band = 0; band < 4; ++band) {
    const uint32_t energy = (uint32_t)(bandSum[band] / frames) * BAND_GAIN[band];
    if (energy > envelope[band]) {
      envelope[band] += (energy - envelope[band] + 1) / 2;
    } else {
      envelope[band] -= (envelope[band] - energy + 5) / 6;
    }
    if (envelope[band] > loudest) loudest = envelope[band];
  }

  if (loudest > sharedPeak) {
    sharedPeak = loudest;
  } else if (sharedPeak > 900) {
    uint32_t decay = sharedPeak / 96;
    if (decay == 0) decay = 1;
    sharedPeak = sharedPeak - 900 > decay ? sharedPeak - decay : 900;
  }

  for (uint8_t band = 0; band < 4; ++band) {
    uint8_t rows = 0;
    if (avg >= 120 && sharedPeak > 0) {
      uint32_t scaled = (envelope[band] * 12U + sharedPeak / 2U) / sharedPeak;
      if (scaled > 12U) scaled = 12U;
      rows = (uint8_t)scaled;
    }
    pcmEqBands[band] = rows;
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
    Serial.print(avg);
    Serial.print(" eq=");
    for (uint8_t band = 0; band < 4; ++band) {
      if (band > 0) Serial.print(',');
      Serial.print(pcmEqBands[band]);
    }
    Serial.println();

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
