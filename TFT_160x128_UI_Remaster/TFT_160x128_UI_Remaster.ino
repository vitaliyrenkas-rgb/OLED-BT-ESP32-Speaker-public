#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "ui_theme.h"
#include "tft_ui_renderer.h"

// OLEG TFT 1.8" 160x128 UI REMASTER — VISUAL SHOWROOM BENCH
// Confirmed bench wiring:
// SCLK=18, MOSI=23, CS=19, DC=22, RST=16, GND=GND, LED=3V3.
// Proven previous smoke-test path: INITR_BLACKTAB, rotation=1, SPI=27MHz.

constexpr int TFT_SCLK = 18;
constexpr int TFT_MOSI = 23;
constexpr int TFT_CS   = 19;
constexpr int TFT_DC   = 22;
constexpr int TFT_RST  = 16;
constexpr int TFT_MISO = -1;
constexpr uint8_t TFT_ROTATION = 1;
constexpr uint32_t TFT_SPI_HZ = 27000000UL;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
TftUi::Renderer ui(tft);
TftUi::Model model;

static constexpr uint32_t SCENE_MS = 30000UL;
static constexpr uint32_t FRAME_MS = 120UL;

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
  { TftUi::SLEEP,   "SUN",   false, "SLEEP KITTY + animated Z" }
};

static constexpr uint8_t DEMO_SCENE_COUNT =
  sizeof(demoScenes) / sizeof(demoScenes[0]);

static uint8_t demoSceneIndex = 0;
static uint32_t sceneStartedMs = 0;
static uint32_t lastFrameMs = 0;

static void loadScene(uint8_t index) {
  const DemoScene &scene = demoScenes[index];

  model.screen = scene.screen;
  model.weatherState = scene.weatherState;
  model.weatherNight = scene.weatherNight;

  model.topTime = "23:47";
  model.temperatureC = 21;
  model.duration = "02:36";
  model.title = "Metallica - One";
  model.artist = "...And Justice for All";
  model.playbackActive = true;
  model.date = "SUN 17 AUG";
  model.weatherFeelsC = 19;
  model.humidity = 64;
  model.volumePercent = 37;

  Serial.printf("[SHOWROOM] %u/%u: %s\n",
                index + 1, DEMO_SCENE_COUNT, scene.serialName);
}

static void animateStatusIcons(uint32_t nowMs) {
  uint8_t phase = (nowMs / 5000UL) % 6;

  model.btConnected = true;
  model.wifiConnected = true;
  model.batteryPresent = true;
  model.batteryCharging = false;
  model.batteryPercent = 82;

  switch (phase) {
    case 0:
      model.batteryPercent = 82;
      break;
    case 1:
      model.batteryCharging = true;
      model.batteryPercent = 64;
      break;
    case 2:
      model.batteryPercent = 12;
      break;
    case 3:
      model.wifiConnected = false;
      model.batteryPercent = 55;
      break;
    case 4:
      model.btConnected = false;
      model.batteryPercent = 55;
      break;
    case 5:
      model.batteryPresent = false;
      model.batteryPercent = 0;
      break;
  }
}

static void animatePlayerEq(uint32_t nowMs) {
  static const uint8_t eqPattern[] = {
    2, 4, 6, 3, 7, 5, 2, 6,
    4, 1, 5, 7, 3, 6, 2, 5
  };

  uint8_t frame = (nowMs / 150UL) %
                  (sizeof(eqPattern) / sizeof(eqPattern[0]));
  model.eqLevel = eqPattern[frame];
}

static void animateVolume(uint32_t nowMs) {
  uint16_t p = (nowMs / 50UL) % 200;
  model.volumePercent = (p <= 100) ? p : 200 - p;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("OLEG TFT 160x128 UI REMASTER / VISUAL SHOWROOM BENCH");
  Serial.println("Pins: SCLK=18 MOSI=23 CS=19 DC=22 RST=16 LED=3V3");
  Serial.println("ST7735: INITR_BLACKTAB, rotation=1, SPI=27MHz");
  Serial.println("Scene duration: 30 seconds");

  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(TFT_ROTATION);
  tft.setSPISpeed(TFT_SPI_HZ);
  tft.fillScreen(TftUiTheme::BG);

  Serial.printf("TFT logical size: %d x %d\n", tft.width(), tft.height());

  ui.begin();
  loadScene(0);
  sceneStartedMs = millis();
}

void loop() {
  uint32_t nowMs = millis();
  model.nowMs = nowMs;

  if (nowMs - sceneStartedMs >= SCENE_MS) {
    demoSceneIndex = (demoSceneIndex + 1) % DEMO_SCENE_COUNT;
    loadScene(demoSceneIndex);
    sceneStartedMs = nowMs;
  }

  if (nowMs - lastFrameMs < FRAME_MS) return;
  lastFrameMs = nowMs;

  animateStatusIcons(nowMs);

  if (model.screen == TftUi::PLAYER) {
    animatePlayerEq(nowMs);
  }

  if (model.screen == TftUi::VOLUME) {
    animateVolume(nowMs);
  }

  ui.draw(model);
}
