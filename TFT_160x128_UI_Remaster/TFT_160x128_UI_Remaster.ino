#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "ui_theme.h"
#include "tft_ui_renderer_approved.h"
#include "kitty_approved_asset.h"

// OLEG TFT 1.8" 160x128 UI REMASTER — MANUAL UI BENCH
// Confirmed bench wiring:
// SCLK=18, MOSI=23, CS=19, DC=22, RST=16, GND=GND, LED=3V3.
// NOB button: GPIO15 <-> GND, INPUT_PULLUP, press = next screen/state.
// Proven display path: INITR_BLACKTAB, rotation=1, SPI=27MHz.

constexpr int TFT_SCLK = 18;
constexpr int TFT_MOSI = 23;
constexpr int TFT_CS   = 19;
constexpr int TFT_DC   = 22;
constexpr int TFT_RST  = 16;
constexpr int TFT_MISO = -1;
constexpr uint8_t TFT_ROTATION = 1;
constexpr uint32_t TFT_SPI_HZ = 27000000UL;

constexpr int NOB_PIN = 15;
constexpr uint32_t NOB_DEBOUNCE_MS = 30UL;
constexpr uint32_t FRAME_MS = 120UL;
constexpr uint32_t SLEEP_Z_ANIMATION_MS = 450UL;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
TftUi::Renderer ui(tft);
TftUi::Model model;

struct DemoScene {
  TftUi::Screen screen;
  const char *weatherState;
  bool weatherNight;
  const char *serialName;
};

static const DemoScene demoScenes[] = {
  { TftUi::PLAYER,  "SUN",   false, "PLAYER + animated brick EQ" },
  { TftUi::CLOCK,   "SUN",   false, "CLOCK" },
  { TftUi::WEATHER, "SUN",   false, "WEATHER / CLEAR DAY" },
  { TftUi::WEATHER, "SUN",   true,  "WEATHER / CLEAR NIGHT" },
  { TftUi::WEATHER, "CLOUD", false, "WEATHER / CLOUD" },
  { TftUi::WEATHER, "RAIN",  false, "WEATHER / RAIN" },
  { TftUi::WEATHER, "SNOW",  false, "WEATHER / SNOW" },
  { TftUi::VOLUME,  "SUN",   false, "VOLUME + animated level" },
  { TftUi::SLEEP,   "SUN",   false, "SLEEP / approved kitty + animated Z" }
};

static constexpr uint8_t DEMO_SCENE_COUNT =
  sizeof(demoScenes) / sizeof(demoScenes[0]);

static uint8_t demoSceneIndex = 0;
static uint32_t lastFrameMs = 0;
static bool sceneNeedsFullDraw = true;

static bool nobRawState = HIGH;
static bool nobStableState = HIGH;
static uint32_t nobRawChangedMs = 0;

static uint8_t lastSleepZFrame = 0xFF;

static void loadScene(uint8_t index) {
  const DemoScene &scene = demoScenes[index];

  model.screen = scene.screen;
  model.weatherState = scene.weatherState;
  model.weatherNight = scene.weatherNight;

  // Stable showroom data. Only the selected scene/state changes on NOB press.
  model.btConnected = true;
  model.wifiConnected = true;
  model.batteryPresent = true;
  model.batteryCharging = false;
  model.batteryPercent = 82;

  model.topTime = "23:47";
  model.temperatureC = 21;
  model.duration = "02:36";
  model.title = "Metallica - One";
  model.artist = "...And Justice for All";
  model.eqLevel = 5;
  model.playbackActive = true;
  model.date = "SUN 17 AUG";
  model.weatherFeelsC = 19;
  model.humidity = 64;
  model.volumePercent = 37;

  sceneNeedsFullDraw = true;
  lastSleepZFrame = 0xFF;

  Serial.printf("[NOB] %u/%u: %s\n",
                index + 1, DEMO_SCENE_COUNT, scene.serialName);
}

static void nextScene() {
  demoSceneIndex = (demoSceneIndex + 1) % DEMO_SCENE_COUNT;
  loadScene(demoSceneIndex);
}

static void pollNob(uint32_t nowMs) {
  const bool raw = digitalRead(NOB_PIN);

  if (raw != nobRawState) {
    nobRawState = raw;
    nobRawChangedMs = nowMs;
  }

  if ((nowMs - nobRawChangedMs) < NOB_DEBOUNCE_MS) return;
  if (nobStableState == nobRawState) return;

  nobStableState = nobRawState;

  if (nobStableState == LOW) {
    nextScene();
  }
}

static void animatePlayerEq(uint32_t nowMs) {
  static const uint8_t eqPattern[] = {
    2, 4, 6, 3, 7, 5, 2, 6,
    4, 1, 5, 7, 3, 6, 2, 5
  };

  const uint8_t frame = (nowMs / 150UL) %
                        (sizeof(eqPattern) / sizeof(eqPattern[0]));
  model.eqLevel = eqPattern[frame];
}

static void animateVolume(uint32_t nowMs) {
  const uint16_t p = (nowMs / 50UL) % 200;
  model.volumePercent = (p <= 100) ? p : 200 - p;
}

static void drawZGlyph(int16_t x, int16_t y, uint8_t scale, uint16_t color) {
  const int16_t w = 5 * scale;
  tft.fillRect(x, y, w, scale, color);
  tft.fillRect(x, y + 4 * scale, w, scale, color);

  for (uint8_t i = 0; i < 4; ++i) {
    tft.fillRect(x + (3 - i) * scale,
                 y + (i + 1) * scale,
                 scale, scale, color);
  }
}

static uint8_t sleepZFrame(uint32_t nowMs) {
  return (nowMs / SLEEP_Z_ANIMATION_MS) % 4;
}

static void drawSleepZFull(uint32_t nowMs) {
  const uint8_t frame = sleepZFrame(nowMs);

  // Small Z is present in every animation frame.
  drawZGlyph(95, 49, 1, TftUiTheme::DIM);

  if (frame >= 1) drawZGlyph(110, 34, 2, TftUiTheme::FG);
  if (frame >= 2) drawZGlyph(130, 13, 3, TftUiTheme::FG);

  lastSleepZFrame = frame;
}

static void drawApprovedSleepScene(uint32_t nowMs) {
  tft.fillScreen(TftUiTheme::BG);
  TftUiAssets::drawApprovedKitty(tft);
  drawSleepZFull(nowMs);
}

static void updateSleepZ(uint32_t nowMs) {
  const uint8_t frame = sleepZFrame(nowMs);
  if (frame == lastSleepZFrame) return;

  // Only medium/large Z animate. Keep the small Z and kitty untouched.
  tft.fillRect(108, 8, 52, 40, TftUiTheme::BG);
  if (frame >= 1) drawZGlyph(110, 34, 2, TftUiTheme::FG);
  if (frame >= 2) drawZGlyph(130, 13, 3, TftUiTheme::FG);

  lastSleepZFrame = frame;
}

static void drawCurrentScene(uint32_t nowMs) {
  if (model.screen == TftUi::SLEEP) {
    if (sceneNeedsFullDraw) {
      drawApprovedSleepScene(nowMs);
      sceneNeedsFullDraw = false;
    } else {
      updateSleepZ(nowMs);
    }
    return;
  }

  if (sceneNeedsFullDraw) {
    ui.invalidate();
    sceneNeedsFullDraw = false;
  }

  ui.draw(model);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("OLEG TFT 160x128 UI REMASTER / MANUAL UI BENCH");
  Serial.println("Pins: SCLK=18 MOSI=23 CS=19 DC=22 RST=16 LED=3V3");
  Serial.println("NOB: GPIO15 -> GND, INPUT_PULLUP, press = next scene/state");
  Serial.println("ST7735: INITR_BLACKTAB, rotation=1, SPI=27MHz");

  pinMode(NOB_PIN, INPUT_PULLUP);
  nobRawState = digitalRead(NOB_PIN);
  nobStableState = nobRawState;
  nobRawChangedMs = millis();

  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(TFT_ROTATION);
  tft.setSPISpeed(TFT_SPI_HZ);
  tft.fillScreen(TftUiTheme::BG);

  Serial.printf("TFT logical size: %d x %d\n", tft.width(), tft.height());

  ui.begin();
  loadScene(0);
}

void loop() {
  const uint32_t nowMs = millis();
  model.nowMs = nowMs;

  pollNob(nowMs);

  if (nowMs - lastFrameMs < FRAME_MS) return;
  lastFrameMs = nowMs;

  // Screen-local animation remains automatic.
  if (model.screen == TftUi::PLAYER) {
    animatePlayerEq(nowMs);
  }

  if (model.screen == TftUi::VOLUME) {
    animateVolume(nowMs);
  }

  drawCurrentScene(nowMs);
}
