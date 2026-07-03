// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= TRACK TIME HELPERS =================
String formatTrackTime(uint32_t ms) {
  uint32_t totalSec = ms / 1000;
  uint16_t minutes = totalSec / 60;
  uint8_t seconds = totalSec % 60;

  char buf[8];
  sprintf(buf, "%02u:%02u", minutes, seconds);
  return String(buf);
}

uint32_t currentTrackPositionMs() {
  if (!btConnected) return 0;
  if (trackTimerRunning) return trackElapsedOffsetMs + (millis() - trackStartedAtMs);
  return trackElapsedOffsetMs;
}

String durationDisplayStr() {
  // FIX v2.10: elapsed only, no total duration.
  return formatTrackTime(currentTrackPositionMs());
}
