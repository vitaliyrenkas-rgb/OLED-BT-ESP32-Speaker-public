#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "ui_theme.h"
#include "tft_ui_renderer.h"

// =============================================================
// OLEG TFT 1.8" 160x128 UI REMASTER — VISUAL SHOWROOM BENCH
// =============================================================
// Confirmed current bench wiring, 2026-08-17:
//   TFT SCK/SCL  -> Lolita GPIO18
//   TFT MOSI/SDA -> Lolita GPIO23
//   TFT CS       -> Lolita GPIO19
//   TFT DC/A0    -> Lolita GPIO22
//   TFT RST/RES  -> Lolita GPIO16
//   TFT GND      -> Lolita GND
//   TFT LED/BL   -> Lolita 3V3
//
// Test orientation: landscape, rotation = 1.
// Exact ST7735 init profile is still deliberately not guessed here.
//
// This remains a VISUAL BENCH:
//   no SD, BT stack, Wi-Fi, battery ADC, buttons, audio or real sleep logic.
// All values below are dummy values used only to exercise the complete UI.
// =============================================================

#define OLEG_TFT_SCLK     18
#define OLEG_TFT_MOSI     23
#define OLEG_TFT_CS       19
#define OLEG_TFT_DC       22
#define OLEG_TFT_RST      16
#define OLEG_TFT_ROTATION 1

#ifndef OLEG_TFT_INITR
  #error "Define OLEG_TFT_INITR to the confirmed Adafruit ST7735 initR profile for this exact module."
#endif

// Software SPI is deliberate for this isolated visual bench.
Adafruit_ST7735 tft(
  OLEG_TFT_CS,
  OLEG_TFT_DC,
  OLEG_TFT_MOSI,
  OLEG_TFT_SCLK,
  OLEG_TFT_RST
);

TftUi::Renderer ui(tft);
TftUi::Model model;

// Every showroom scene remains visible for 30 seconds.
static constexpr uint32_t SCENE_MS = 30000UL;
static constexpr uint32_t FRAME_MS = 120UL;

struct DemoScene {
  TftUi::Screen screen;
  const char *weatherState;
  bool weatherNight;
  const char *serialName;
};

// Full visual catalog approved for this TFT remaster.
// Weather variants are separate scenes so every icon can be judged calmly.
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

  // Common dummy content mirrors the real OLEG data hierarchy.
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
  // Six 5-second states fit exactly inside each 30-second scene.
  // This exercises all top-bar semantic states without adding fake screens.
  uint8_t phase = (nowMs / 5000UL) % 6;

  model.btConnected = true;
  model.wifiConnected = true;
  model.batteryPresent = true;
  model.batteryCharging = false;
  model.batteryPercent = 82;

  switch (phase) {
    case 0: // normal / healthy
      model.batteryPercent = 82;
      break;

    case 1: // charging lightning
      model.batteryCharging = true;
      model.batteryPercent = 64;
      break;

    case 2: // low-battery color
      model.batteryPercent = 12;
      break;

    case 3: // Wi-Fi crossed
      model.wifiConnected = false;
      model.batteryPercent = 55;
      break;

    case 4: // Bluetooth disconnected label
      model.btConnected = false;
      model.batteryPercent = 55;
      break;

    case 5: // battery unavailable slash
      model.batteryPresent = false;
      model.batteryPercent = 0;
      break;
  }
}

static void animatePlayerEq(uint32_t nowMs) {
  // Non-linear dummy PCM pattern: intentionally looks less like a sawtooth.
  static const uint8_t eqPattern[] = {
    2, 4, 6, 3, 7, 5, 2, 6,
    4, 1, 5, 7, 3, 6, 2, 5
  };

  uint8_t frame = (nowMs / 150UL) %
                  (sizeof(eqPattern) / sizeof(eqPattern[0]));
  model.eqLevel = eqPattern[frame];
}

static void animateVolume(uint32_t nowMs) {
  // 0 -> 100 -> 0 ping-pong over roughly 10 seconds.
  uint16_t p = (nowMs / 50UL) % 200;
  model.volumePercent = (p <= 100) ? p : 200 - p;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("OLEG TFT 160x128 UI REMASTER / VISUAL SHOWROOM BENCH");
  Serial.println("Dummy UI only: no buttons / BT / Wi-Fi / audio / ADC / SD.");
  Serial.println("Pins: SCLK=18 MOSI=23 CS=19 DC=22 RST=16, LED=3V3");
  Serial.println("Landscape rotation: 1");
  Serial.println("Scene duration: 30 seconds");

  tft.initR(OLEG_TFT_INITR);
  tft.setRotation(OLEG_TFT_ROTATION);
  tft.fillScreen(TftUiTheme::BG);

  Serial.printf("TFT logical size: %d x %d\n", tft.width(), tft.height());

  if (tft.width() != 160 || tft.height() != 128) {
    Serial.println("STOP: logical canvas is not 160x128. Fix init profile before judging the UI.");
  }

  ui.begin();

  loadScene(0);
  sceneStartedMs = millis();
  lastFrameMs = 0;
}

void loop() {
  uint32_t nowMs = millis();
  model.nowMs = nowMs;

  // Advance to the next visual scene every 30 seconds.
  if (nowMs - sceneStartedMs >= SCENE_MS) {
    demoSceneIndex = (demoSceneIndex + 1) % DEMO_SCENE_COUNT;
    loadScene(demoSceneIndex);
    sceneStartedMs = nowMs;
  }

  if (nowMs - lastFrameMs < FRAME_MS) return;
  lastFrameMs = nowMs;

  // Top-bar icons cycle through all their visual states on every screen
  // where the top bar exists. Sleep and Volume simply ignore these fields.
  animateStatusIcons(nowMs);

  if (model.screen == TftUi::PLAYER) {
    animatePlayerEq(nowMs);
  }

  if (model.screen == TftUi::VOLUME) {
    animateVolume(nowMs);
  }

  // Sleep Z animation is generated inside the renderer from model.nowMs.
  ui.draw(model);
}
