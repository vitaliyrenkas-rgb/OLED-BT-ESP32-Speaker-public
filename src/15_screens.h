// RT-003 v5.0 — runtime adapter for the approved 160x128 TFT UI.

TftUi::Model tftModel;

enum StandaloneTftScene : uint8_t {
  TFT_STANDALONE_NONE,
  TFT_STANDALONE_GREETING,
  TFT_STANDALONE_MESSAGE,
  TFT_STANDALONE_LOW_BATTERY
};

StandaloneTftScene activeStandaloneTftScene = TFT_STANDALONE_NONE;
int lastStandaloneBatteryPercent = -1;

String playerPrimaryLine() {
  if (artist.length() > 0 && title.length() > 0) return artist + " - " + title;
  if (title.length() > 0) return title;
  return artist;
}

String playerSecondaryLine() {
  if (album.length() > 0) return album;
  return artist;
}

void loadTftRuntimeModel() {
  tftModel.btConnected = btConnected;
  tftModel.wifiConnected = wifiLastSyncOk;
  tftModel.batteryPresent = batteryPresent;
  tftModel.batteryCharging = batteryCharging;
  tftModel.batteryPercent = constrain(batteryPercent, 0, 100);
  tftModel.topTime = timeStr();
  tftModel.temperatureC = (int)weatherTemp;
  tftModel.duration = durationDisplayStr();
  tftModel.title = playerPrimaryLine();
  tftModel.artist = playerSecondaryLine();
  tftModel.eqLevel = constrain(audioLevelToBricks(), 0, 7);
  tftModel.playbackActive = playbackActive;
  tftModel.date = dateStr();
  tftModel.date.toUpperCase();
  tftModel.weatherState = weatherState;
  tftModel.weatherNight = weatherIsNight;
  tftModel.weatherFeelsC = (int)weatherFeels;
  tftModel.humidity = weatherHumidity;
  tftModel.volumePercent = constrain(volumeOverlayPercent, 0, 100);
  tftModel.nowMs = millis();
}

bool beginStandaloneTftScreen(StandaloneTftScene scene) {
  if (activeStandaloneTftScene == scene) return false;
  activeStandaloneTftScene = scene;
  tftUi.invalidate();
  tft.fillScreen(TftUiTheme::BG);
  return true;
}

void leaveStandaloneTftScreen() {
  if (activeStandaloneTftScene == TFT_STANDALONE_NONE) return;
  activeStandaloneTftScene = TFT_STANDALONE_NONE;
  tftUi.invalidate();
}

void drawGreetingScreen() {
  if (!beginStandaloneTftScreen(TFT_STANDALONE_GREETING)) return;
  tft.drawRect(8, 18, 144, 92, TftUiTheme::FG);
  tft.drawRect(10, 20, 140, 88, TftUiTheme::DIM);
  centerText("Welcome", 48, u8g2_font_7x14B_tf);
  centerText(speakerConfig.welcomeText, 76, u8g2_font_7x14_tf);
  centerText(BUILD_VERSION, 99, u8g2_font_5x8_tf);
}

void drawMessageScreen() {
  if (!beginStandaloneTftScreen(TFT_STANDALONE_MESSAGE)) return;
  tft.drawRect(8, 18, 144, 92, TftUiTheme::FG);
  centerText("Bluetooth", 52, u8g2_font_7x14B_tf);
  centerText("is not connected", 78, u8g2_font_6x12_tf);
}

void drawLowBatteryOverlayScreen() {
  const int shownPercent = constrain(batteryPercent, 0, 100);
  const bool entered = beginStandaloneTftScreen(TFT_STANDALONE_LOW_BATTERY);

  if (entered) {
    tft.drawRect(3, 3, 154, 122, TftUiTheme::RED);
    tft.drawRect(5, 5, 150, 118, TftUiTheme::DIM);
    centerText("LOW BATTERY", 34, u8g2_font_7x14B_tf);
    tft.drawFastHLine(13, 42, 134, TftUiTheme::RED);
  }

  if (!entered && shownPercent == lastStandaloneBatteryPercent) return;
  tft.fillRect(20, 50, 120, 49, TftUiTheme::BG);
  centerText(String(shownPercent) + "%", 90, u8g2_font_logisoso42_tn);
  lastStandaloneBatteryPercent = shownPercent;
}

void drawUI() {
  loadTftRuntimeModel();

  if (lowBatteryWarningOverlayActive) {
    drawLowBatteryOverlayScreen();
    return;
  }

  if (currentScreen == SCREEN_GREETING) {
    drawGreetingScreen();
    return;
  }

  if (currentScreen == SCREEN_MESSAGE) {
    drawMessageScreen();
    return;
  }

  leaveStandaloneTftScreen();

  if (volumeOverlayActive) {
    tftModel.screen = TftUi::VOLUME;
  } else if (currentScreen == SCREEN_PLAYER) {
    tftModel.screen = TftUi::PLAYER;
  } else if (currentScreen == SCREEN_WEATHER) {
    tftModel.screen = TftUi::WEATHER;
  } else if (currentScreen == SCREEN_SLEEP) {
    tftModel.screen = TftUi::SLEEP;
  } else {
    tftModel.screen = TftUi::CLOCK;
  }

  tftUi.draw(tftModel);
}
