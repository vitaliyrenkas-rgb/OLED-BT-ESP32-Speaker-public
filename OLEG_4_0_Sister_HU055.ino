#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <sys/time.h>
#include <Preferences.h>
#include "nvs_flash.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
// #include "driver/i2s.h"
#include <math.h>

// =====================================================
// RT-003 v5.0 — ESP32 LoLin32 / HU-055 / 1.8-inch ST7735.
// Firmware build 5.0-016. Real hardware only; not for Wokwi.
// Structured as one Arduino translation unit through ordered src headers.
// =====================================================

#include "src/00_config.h"
#include "src/ui_theme.h"
#include "src/kitty_approved_asset.h"
#include "src/tft_ui_renderer_approved.h"
#include "src/01_display_pins.h"
#include "src/02_audio_pins.h"
#include "src/03_button_pins.h"
#include "src/04_battery_pins.h"
#include "src/04_power_switch.h"
#include "src/05_state.h"
#include "src/06_text_helpers.h"
#include "src/07_config_language.h"
#include "src/08_battery.h"
#include "src/09_wifi_weather.h"
// Legacy SSD1309 renderers are retained in src/ for rollback/reference only.
// #include "src/10_icons.h"
// #include "src/11_common_ui.h"
#include "src/12_playback_pcm.h"
#include "src/13_track_time.h"
// #include "src/14_player_helpers.h"  // superseded by TFT renderer
#include "src/15_screens.h"
#include "src/16_buttons.h"
#include "src/17_callbacks.h"
#include "src/18_diagnostics.h"
#include "src/19_config_reset.h"
#include "src/20_startup_jingle_disabled.h"
#include "src/90_setup.h"
#include "src/99_loop.h"
