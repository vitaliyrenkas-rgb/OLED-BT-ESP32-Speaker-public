// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= CALLBACKS =================
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  String value = String((const char*)text);

  if (id == ESP_AVRC_MD_ATTR_TITLE) {
    // FIX v2.10:
    // Reset fallback timer whenever TITLE is received again.
    // Covers next track and often repeat-one restart.
    title = value;
    titleOffset = 0;
    resetTrackTimer();
    requestRedraw();
  }

  if (id == ESP_AVRC_MD_ATTR_ARTIST) {
    artist = value;
    artistOffset = 0;
    requestRedraw();
  }

  if (id == ESP_AVRC_MD_ATTR_ALBUM) {
    album = value;
  }

  if (id == ESP_AVRC_MD_ATTR_PLAYING_TIME) {
    // FIX v2.8:
    // AVRCP playing time is usually provided as a string in milliseconds.
    trackDurationMs = (uint32_t)value.toInt();
    trackStartedAtMs = millis();
    trackElapsedOffsetMs = 0;
    requestRedraw();
  }
}

void avrc_connection_state_callback(bool connected) {
  btConnected = connected;

  if (connected) {
    lastUserInteractionMs = millis();
    trackStartedAtMs = millis();
    trackElapsedOffsetMs = 0;

    if (!btAutoSwitchedToPlayer && !manualScreenLock && currentScreen != SCREEN_GREETING) {
      currentScreen = SCREEN_PLAYER;
      btAutoSwitchedToPlayer = true;
    }
  } else {
    title = "Очікування...";
    artist = "Підключи телефон";
    album = "";
    trackDurationMs = 0;
    trackStartedAtMs = 0;
    trackElapsedOffsetMs = 0;
    playbackActive = false;
    trackTimerRunning = false;
    pcmLevelRaw = 0;
    titleOffset = 0;
    artistOffset = 0;
    btAutoSwitchedToPlayer = false;
    manualScreenLock = false;

    if (currentScreen != SCREEN_GREETING) currentScreen = SCREEN_CLOCK;
  }

  requestRedraw();
}
