#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "ui_theme.h"
#include "tft_ui_renderer.h"

// =============================================================
// OLEG TFT 1.8" 160x128 UI REMASTER — ISOLATED VISUAL TEST
// =============================================================
// This sketch intentionally contains NO guessed wiring and NO guessed
// ST7735 init profile. Fill these only from confirmed physical wiring/module.
//
// Required libraries:
//   Adafruit GFX Library
//   Adafruit ST7735 and ST7789 Library
//   U8g2_for_Adafruit_GFX
//
// This is UI-only: no SD, BT, Wi-Fi, battery ADC, sleep logic or audio.
// =============================================================

#ifndef OLEG_TFT_CS
  #error "Define OLEG_TFT_CS from confirmed TFT wiring."
#endif
#ifndef OLEG_TFT_DC
  #error "Define OLEG_TFT_DC from confirmed TFT wiring."
#endif
#ifndef OLEG_TFT_RST
  #error "Define OLEG_TFT_RST from confirmed TFT wiring (or -1 only if physically confirmed appropriate)."
#endif
#ifndef OLEG_TFT_MOSI
  #error "Define OLEG_TFT_MOSI from confirmed TFT wiring."
#endif
#ifndef OLEG_TFT_SCLK
  #error "Define OLEG_TFT_SCLK from confirmed TFT wiring."
#endif
#ifndef OLEG_TFT_INITR
  #error "Define OLEG_TFT_INITR to the confirmed Adafruit ST7735 initR profile for this module."
#endif
#ifndef OLEG_TFT_ROTATION
  #error "Define OLEG_TFT_ROTATION after confirming the landscape orientation on this module."
#endif

// Software-SPI constructor is deliberate for the first isolated bench:
// it binds ONLY to the confirmed pins above and does not depend on board-default SPI pins.
Adafruit_ST7735 tft(
  OLEG_TFT_CS,
  OLEG_TFT_DC,
  OLEG_TFT_MOSI,
  OLEG_TFT_SCLK,
  OLEG_TFT_RST
);

TftUi::Renderer ui(tft);
TftUi::Model model;

static uint32_t lastScreenChangeMs = 0;
static uint8_t demoScreenIndex = 0;

static const TftUi::Screen demoScreens[] = {
  TftUi::PLAYER,
  TftUi::CLOCK,
  TftUi::WEATHER,
  TftUi::VOLUME,
  TftUi::SLEEP
};

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("OLEG TFT 160x128 UI REMASTER / isolated visual test");
  Serial.println("No BT / Wi-Fi / SD / battery ADC / audio in this sketch.");

  tft.initR(OLEG_TFT_INITR);
  tft.setRotation(OLEG_TFT_ROTATION);
  tft.fillScreen(TftUiTheme::BG);

  Serial.printf("TFT logical size after init/rotation: %d x %d\n",
                tft.width(), tft.height());

  if (tft.width() != 160 || tft.height() != 128) {
    Serial.println("STOP: logical canvas is not 160x128. Check init profile/rotation before judging UI layout.");
  }

  ui.begin();

  model.screen = TftUi::PLAYER;
  model.topTime = "23:47";
  model.temperatureC = 21;
  model.batteryPercent = 82;
  model.btConnected = true;
  model.wifiConnected = true;
  model.duration = "02:36";
  model.title = "Metallica - One";
  model.artist = "...And Justice for All";
  model.eqLevel = 5;
  model.playbackActive = true;
  model.date = "SUN 17 AUG";
  model.weatherState = "SUN";
  model.weatherNight = true;
  model.weatherFeelsC = 19;
  model.humidity = 64;
  model.volumePercent = 37;

  ui.draw(model);
  lastScreenChangeMs = millis();
}

void loop() {
  model.nowMs = millis();

  // Tiny fake EQ motion only for visual evaluation.
  if (model.screen == TftUi::PLAYER) {
    model.eqLevel = 2 + ((millis() / 180UL) % 6);
    ui.draw(model);
    delay(90);
    return;
  }

  // Sleep Z animation needs redraws too.
  if (model.screen == TftUi::SLEEP) {
    ui.draw(model);
    delay(120);
  }

  if (millis() - lastScreenChangeMs >= 4000UL) {
    demoScreenIndex = (demoScreenIndex + 1) %
                      (sizeof(demoScreens) / sizeof(demoScreens[0]));
    model.screen = demoScreens[demoScreenIndex];
    ui.draw(model);
    lastScreenChangeMs = millis();
  }
}
