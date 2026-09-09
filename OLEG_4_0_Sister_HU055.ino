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
// #include "driver/i2s.h"
#include <math.h>
  
//  8) UI patch v2.8:
//     - Wi-Fi icon redrawn smaller and kept above topbar line.
//     - language selection screen simplified: En left, Укр. right.
//     - Welcome screen supports both languages and uses two centered lines.
//     - metadata playing time support added via ESP_AVRC_MD_ATTR_PLAYING_TIME.
//     - old vertical equalizer replaced with brick equalizer around duration.
//     - marquee speed increased to 120 ms.
//     - config reset button can reset language/config both at boot and during runtime.
//  9) UI/reset patch v2.9:
//     - config reset moved from GPIO32/D32 to GPIO32/D32.
//     - reset button remains INPUT_PULLUP: D19 -> button -> GND.
//     - debug prints human-readable pressed state: pressed=1, not pressed=0.
//     - Wi-Fi icon redrawn as 3 simple arcs; no Wi-Fi = arcs crossed by slash.
//     - Player redraw sped up for smoother timer/EQ.
//     - track timer displays elapsed only: 00:35.
//     - fallback timer resets on title callback/reconnect and freezes on disconnect.
//  10) Playback/NVS patch v2.10:
//      - real PCM tap via a2dp_sink.set_stream_reader() added.
//      - brick equalizer now follows real audio level, not fake millis animation.
//      - timer and EQ pause when playback stops/pauses or PCM goes silent.
//      - track timer displays elapsed only: 00:35.
//      - fallback timer resets on TITLE callback / track restart.
//      - config reset moved to GPIO32/D32 and hard-erases NVS via nvs_flash_erase().
//      - extra fallback config reset combo: BTN_CLOCK + BTN_WEATHER hold 3 sec.
//      - Wi-Fi icon moved upward and redrawn as simple 3 arcs + slash when sync failed.
//  11) UI polish v2.11:
//      - Wi-Fi icon redrawn in cleaner pixel-art style.
//      - Wi-Fi icon aligned vertically with Bluetooth icon.
//      - EQ upgraded from 2 to 3 brick columns per side.
//      - EQ sensitivity increased for more lively movement.
//      - config reset pin moved from GPIO32/D32 to GPIO32/D32.
//  12) Safe config reset v2.12:
//      - runtime hard reset disabled to stop automatic reset loop.
//      - config reset is boot-only and requires stable D32 LOW hold.
//      - language screen no longer blocks forever; defaults to UA after timeout.
//      - language label uses ASCII 'UA' instead of Cyrillic 'Укр.' to avoid dot/missing glyph.
//  13) Language/weather UI patch v2.13:
//      - external hard reset button removed from runtime flow.
//      - config/language reset now uses BTN_PLAYER + BTN_WEATHER hold 5 sec.
//      - after combo reset, language selection screen is shown without hard reboot.
//      - nav label 'Пог.' changed to 'Погода'.
//      - Weather 'Відч.' uses Cyrillic-capable font.
//      - weatherState label localized: СОНЦЕ / ХМАРИ / ДОЩ / СНІГ.
//  14) Stable merge v3.0:
//      - v2.13 accepted as stable base.
//      - Ukrainian CLOUD label changed from ХМАРИ to ХМАРНО.
//      - Bluetooth icon made smaller to avoid touching topbar line.
//      - brick equalizer sensitivity increased.
//      - normal screen switching blocked while BTN_PLAYER + BTN_WEATHER reset combo is held.
//      - UX: Bluetooth connect auto-switches to Player.
//      - UX: if music is playing and user stays on Clock/Weather, UI returns to Player after 1 min idle.
//  15) v3.2 startup jingle + cosmetics:
//      - startup hi-fi/car-head-unit style jingle added.
//      - jingle plays through MAX98357A before Bluetooth A2DP starts.
//      - Ukrainian clear-weather label changed from СОНЦЕ to ЯСНО.
//      - brick equalizer raised by 2 px to avoid Cyrillic metadata overlap.
//  16) v3.3 excluded jingle + AudioLibraries replacement:
//      - replaced AUDIO Libraries due to IDE update
//      - decomposed single .ino-file into git-structured project
//      - PROD DEVICE TEST: ESP32 LoLin MicroPython with battery port. (PIN 23 replaced PIN 17 on DevKit for SCK on OLED)
// =====================================================
//  VITALIK SPEAKER — HARD / REAL DEVICE SKETCH v3.2 JINGLE + UI COSMETICS
//  Real ESP32 only. NOT for Wokwi.
//
//  FIXES / CHANGES IN v2.7:
//  1) UI + Wi-Fi icon:
//     - compact Wi-Fi icon added near Bluetooth icon in topbar.
//     - filled icon = last Wi-Fi/NTP/weather sync OK.
//     - outline icon = Wi-Fi sync not available / failed.
//  2) NTP/weather without BT conflict:
//     - Wi-Fi is used only during startup sync/weather update.
//     - Wi-Fi is fully turned OFF before A2DP Bluetooth starts.
//     - ensureWiFi() is NOT used in loop.
//     - periodic weather refresh is disabled during active Bluetooth.
//  3) Cyrillic:
//     - date/calendar uses Ukrainian short day/month names.
//     - metadata title/artist use Cyrillic-capable U8g2 fonts.
//  4) Weather UI:
//     - filled cloud icon kept.
//     - weather icon at y=16.
//     - weatherState label under icon kept.
//  5) Battery unavailable icon:
//     - no X.
//     - one diagonal slash top-right -> bottom-left.
//  6) Heap/redraw diagnostics:
//     - Serial heap diagnostics every 10 seconds.
//     - OLED full redraw throttled to reduce audio hiccups.
//  7) Config / language:
//     - Preferences/NVS with CONFIG_VERSION.
//     - first boot / config version change shows Select Language screen.
//     - BTN_PLAYER = En, BTN_WEATHER = Укр.
//     - separate config reset button: GPIO32 / D32.
//       IMPORTANT: classic ESP32 has no GPIO192.
// ===========c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\05_state.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\06_text_helpers.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\07_config_language.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\08_battery.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\09_wifi_weather.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\10_icons.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\11_common_ui.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\12_playback_pcm.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\13_track_time.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\14_player_helpers.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\15_screens.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\16_buttons.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\17_callbacks.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\18_diagnostics.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\19_config_reset.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\20_startup_jingle_disabled.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\90_setup.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\99_loop.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\00_config.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\01_display_pins.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\02_audio_pins.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\03_button_pins.h c:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_BT_Speaker_LoLin_structured\OLEG_BT_Speaker_LoLin\src\04_battery_pins.h==========================================

// =====================================================
// Structured Arduino sketch entry point.
// Implementation is split into ordered headers under src/.
// This keeps the original single-translation-unit behavior stable.
// =====================================================

#include "src/00_config.h"
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
#include "src/90_setup.h"
#include "src/99_loop.h"
