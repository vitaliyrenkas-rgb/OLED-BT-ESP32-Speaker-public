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

String playerTitleLine() {
  if (title.length() > 0) return title;
  return uiLang == LANG_UA ? "БЕЗ НАЗВИ" : "UNTITLED";
}

String playerArtistLine() {
  if (artist.length() > 0) return artist;
  return album;
}

void loadTftCalendarStrings(String &weekday, String &fullDate) {
  struct tm t;
  if (!getLocalTime(&t, 5) || t.tm_year < (2024 - 1900)) {
    weekday = uiLang == LANG_UA ? "ЧАС НЕ СИНХР." : "TIME NOT SYNC";
    fullDate = "";
    return;
  }

  if (uiLang == LANG_UA) {
    static const char* DAYS[] = {
      "НЕДІЛЯ", "ПОНЕДІЛОК", "ВІВТОРОК", "СЕРЕДА",
      "ЧЕТВЕР", "П'ЯТНИЦЯ", "СУБОТА"
    };
    static const char* MONTHS[] = {
      "СІЧНЯ", "ЛЮТОГО", "БЕРЕЗНЯ", "КВІТНЯ",
      "ТРАВНЯ", "ЧЕРВНЯ", "ЛИПНЯ", "СЕРПНЯ",
      "ВЕРЕСНЯ", "ЖОВТНЯ", "ЛИСТОПАДА", "ГРУДНЯ"
    };
    weekday = DAYS[t.tm_wday];
    fullDate = String(t.tm_mday) + " " + MONTHS[t.tm_mon] + " " +
               String(t.tm_year + 1900);
  } else {
    static const char* DAYS[] = {
      "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
      "THURSDAY", "FRIDAY", "SATURDAY"
    };
    static const char* MONTHS[] = {
      "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
      "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
    weekday = DAYS[t.tm_wday];
    fullDate = String(t.tm_mday) + " " + MONTHS[t.tm_mon] + " " +
               String(t.tm_year + 1900);
  }
}

void loadTftRuntimeModel() {
  tftModel.ukrainian = uiLang == LANG_UA;
  tftModel.btConnected = btConnected;
  tftModel.wifiConnected = wifiLastSyncOk;
  tftModel.batteryPresent = batteryPresent;
  tftModel.batteryCharging = batteryCharging;
  tftModel.batteryPercent = constrain(batteryPercent, 0, 100);
  tftModel.batteryIconPercent = getBatteryIconPercent();
  tftModel.topTime = timeStr();
  tftModel.temperatureC = (int)weatherTemp;
  tftModel.duration = durationDisplayStr();
  tftModel.title = playerTitleLine();
  tftModel.artist = playerArtistLine();
  for (uint8_t band = 0; band < 4; ++band) {
    const uint8_t rows = pcmEqBands[band];
    tftModel.eqBands[band] = playbackActive
      ? constrain(rows, 0, 12)
      : 0;
  }
  tftModel.playbackActive = playbackActive;
  loadTftCalendarStrings(tftModel.weekday, tftModel.date);
  tftModel.weatherState = weatherState;
  tftModel.weatherNight = weatherIsNight;
  if (weatherIconPreviewIndex >= 0) {
    switch (weatherIconPreviewIndex) {
      case 0:
        tftModel.weatherState = "SUN";
        tftModel.weatherNight = false;
        break;
      case 1:
        tftModel.weatherState = "SUN";
        tftModel.weatherNight = true;
        break;
      case 2:
        tftModel.weatherState = "CLOUD";
        tftModel.weatherNight = false;
        break;
      case 3:
        tftModel.weatherState = "RAIN";
        tftModel.weatherNight = false;
        break;
      case 4:
      default:
        tftModel.weatherState = "SNOW";
        tftModel.weatherNight = false;
        break;
    }
  }
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
  centerText(speakerConfig.welcomeLine1, 48, u8g2_font_6x12_t_cyrillic);
  centerText(speakerConfig.welcomeLine2, 76, u8g2_font_6x12_t_cyrillic);
  centerText(BUILD_VERSION, 99, u8g2_font_5x8_tf);
}

void drawMessageScreen() {
  if (!beginStandaloneTftScreen(TFT_STANDALONE_MESSAGE)) return;
  tft.drawRect(8, 18, 144, 92, TftUiTheme::FG);
  centerText("Bluetooth", 52, u8g2_font_7x14B_tf);
  centerText(uiLang == LANG_UA ? "не підключено" : "is not connected", 78,
             u8g2_font_6x12_t_cyrillic);
}

void drawLowBatteryOverlayScreen() {
  const int shownPercent = constrain(batteryPercent, 0, 100);
  const bool entered = beginStandaloneTftScreen(TFT_STANDALONE_LOW_BATTERY);

  if (entered) {
    tft.drawRect(3, 3, 154, 122, TftUiTheme::RED);
    tft.drawRect(5, 5, 150, 118, TftUiTheme::DIM);
    centerText(uiLang == LANG_UA ? "НИЗЬКИЙ ЗАРЯД" : "LOW BATTERY", 34,
               uiLang == LANG_UA ? u8g2_font_6x12_t_cyrillic
                                 : u8g2_font_7x14B_tf);
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
