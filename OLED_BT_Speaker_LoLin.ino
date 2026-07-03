// =====================================================
// OLED BT Speaker — structured Arduino sketch entry point.
// Implementation is split into ordered headers under src/.
// This keeps the original single-translation-unit Arduino behavior stable.
// =====================================================

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Preferences.h>
#include "nvs_flash.h"
#include <U8g2lib.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include <math.h>

#include "src/00_config.h"
#include "src/01_display_pins.h"
#include "src/02_audio_pins.h"
#include "src/03_button_pins.h"
#include "src/04_battery_pins.h"
#include "src/05_state.h"
#include "src/06_text_helpers.h"
#include "src/07_config_language.h"
#include "src/08_battery.h"
#include "src/09_wifi_weather.h"
#include "src/10_icons.h"
#include "src/11_common_ui.h"
#include "src/12_playback_pcm.h"
#include "src/13_track_time.h"
#include "src/14_player_helpers.h"
#include "src/15_screens.h"
#include "src/16_buttons.h"
#include "src/17_callbacks.h"
#include "src/18_diagnostics.h"
#include "src/19_config_reset.h"
#include "src/20_startup_jingle_disabled.h"
#include "src/21_sleep_kitty_bitmap.h"
#include "src/90_setup.h"
#include "src/99_loop.h"
