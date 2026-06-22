// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= LOOP =================
void loop() {
  unsigned long now = millis();

  if (currentScreen == SCREEN_GREETING && now >= greetingUntil) {
    currentScreen = btConnected ? SCREEN_PLAYER : SCREEN_CLOCK;
    requestRedraw();
  }

  // No background Wi-Fi here.
  // It caused A2DP stutter in previous builds.
  handleConfigResetButton();
  handleButtons();

  // FIX v3.0:
  // If Bluetooth is connected and music is playing, but user left UI on Clock/Weather,
  // return to Player after 1 minute of no button activity.
  // OLEG v3.5-003:
  // Weather is a quick-look screen: if BT is connected, return to Player after 1 minute
  // even if playback is paused. Clock keeps the older playback-active behavior.
  bool autoReturnFromClock = btConnected && playbackActive && currentScreen == SCREEN_CLOCK;
  bool autoReturnFromWeatherToPlayer = btConnected && currentScreen == SCREEN_WEATHER;
  bool autoReturnFromWeatherToClock = !btConnected && currentScreen == SCREEN_WEATHER;

  if ((autoReturnFromClock || autoReturnFromWeatherToPlayer || autoReturnFromWeatherToClock) &&
      now - lastUserInteractionMs > 60000) {
    currentScreen = autoReturnFromWeatherToClock ? SCREEN_CLOCK : SCREEN_PLAYER;
    manualScreenLock = false;
    lastUserInteractionMs = now;
    requestRedraw();
  }



  // FIX v2.10:
  // If AVRCP playstatus is not delivered by phone/browser, infer pause/stop from PCM silence.
  if (btConnected && playbackActive && millis() - lastPcmAudioMs > 1200) {
    setPlaybackActive(false);
  }

  // If PCM resumes without AVRCP status, resume timer/EQ.
  if (btConnected && !playbackActive && millis() - lastPcmAudioMs < 500 && pcmLevelRaw > 250) {
    setPlaybackActive(true);
  }



  if (now - lastBattery > 10000) {
    lastBattery = now;
    updateBattery();
    requestRedraw();
  }

  // Periodic weather refresh.
  // Safe v3.3 policy: refresh only when Bluetooth is not connected,
  // because active Wi-Fi caused A2DP stutter in previous builds.
  if (!btConnected && now - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
  updateWeatherCycle();
  }

  if (now - lastHeapLog > 10000) {
    lastHeapLog = now;
    logHeapDiagnostics();
  }

  // Redraw throttle:
  // - Player with BT: slower, to reduce I2C/OLED pressure during A2DP.
  // - forced redraw still happens immediately after button/metadata changes.
  unsigned long refreshRate = 1000;
  if (btConnected && currentScreen == SCREEN_PLAYER) refreshRate = 120; // FIX v2.9: smooth timer/EQ
  if (currentScreen == SCREEN_CLOCK) refreshRate = 1000;
  if (currentScreen == SCREEN_WEATHER) refreshRate = 1000;

  if (forceRedraw || now - lastDraw > refreshRate) {
    forceRedraw = false;
    lastDraw = now;
    drawUI();
  }
}
