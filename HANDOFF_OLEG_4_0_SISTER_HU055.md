# HANDOFF — OLEG 4.0 Sister Edition / HU-055

Дата: 2026-07-17  
Проєкт: **OLEG 4.0 Sister Edition**  
Стан: окрему робочу копію й гілку вже відведено.

---

## 0. Стиль роботи

- Українською, коротко й по ділу.
- Не розганятись у “почнемо з нуля”.
- Не пропонувати повторний hardware/audio proof, якщо немає нового симптому.
- Не давати патчі “по памʼяті”: перед серйозними змінами брати актуальний `src.zip`.
- У vibe-моментах не вмикати checklist mode. На “нормально делай?” відповідь: **“нормально будєт!”**

---

## 1. Що вже зроблено перед новим чатом

Створена окрема робоча копія:

```text
C:\Users\Admin\Documents\Arduino\DIY_BT_Speaker_OLED_1_3_UI_v2\OLED LolinESP32\OLEG_4_0_Sister_HU055
```

Створена гілка:

```text
feature/oleg-4-sister-hu055
```

База гілки:

```text
4b3114c Add low battery warning
```

Поточний log на старті:

```text
4b3114c (HEAD -> feature/oleg-4-sister-hu055, origin/feature/v3.5-config-portal, origin/HEAD, feature/v3.5-config-portal) Add low battery warning
7c4d8ab (tag: oleg-v3.5-012-sleep-kitty) Add sleep kitty screen
d3a14c3 (tag: oleg-v3.5-010-battery-pot) Calibrate battery and smooth volume pot
2fedf4b (tag: oleg-v3.5-009-config-portal) Add AP config portal
2d08e68 Wire runtime to speaker config
```

Гілку запушено в `origin`, але важливо:

```text
origin зараз — локальна батьківська dev-репа,
бо clone зроблено з локального шляху, а не з GitHub URL.
```

Команда, яка була виконана:

```bash
git clone "Vitalik_Speaker_HARD_v3_2jingle_ui_cosmetic_LoLin32_MicroPython" "OLEG_4_0_Sister_HU055"
cd "OLEG_4_0_Sister_HU055"
git switch -c feature/oleg-4-sister-hu055
git push -u origin feature/oleg-4-sister-hu055
```

---

## 2. Важливий контекст попереднього OLEG v3.5.x

OLEG / OLED BT Speaker v3.5.x уже має:

```text
✅ Bluetooth A2DP audio baseline
✅ OLED UI: Player / Clock / Weather
✅ Wi‑Fi + NTP + OpenWeather
✅ SpeakerConfig / NVS
✅ AP Config Portal
✅ battery / VBUS / charging indication
✅ GPIO35 volume pot + overlay
✅ sleep kitty screen
✅ low battery warning v3.5-013
```

Останній dev commit, від якого стартує OLEG 4.0:

```text
4b3114c Add low battery warning
```

---

## 3. Що таке OLEG 4.0

OLEG 4.0 — це **не перепис софту** і не новий UX з нуля.

Це:

```text
той самий софтовий мозок v3.5.x
+
інший hardware target
```

Чому major:

```text
v3.5.x = перший корпус / OLED 128×64 / MAX98357A / поточна Лоліта
v4.0 = подарунковий hardware target / transparent OLED / інший audio chain / HU-055 donor
```

---

## 4. Що вже НЕ треба робити

Не треба знову:

```text
❌ фоткати HU-055 як перший discovery-step
❌ шукати L/R/GND з нуля
❌ перевіряти analog input proof
❌ доводити, що PCM5102A може подати звук
```

Це вже зроблено / узгоджено.

Актуальна база по залізу:

```text
✅ HU-055 плата/корпус — донор
✅ analog L/R/GND точки вже знайдені
✅ шлях PCM5102A → штатний analog input HU-055 вже зрозумілий
✅ штатний BT не використовується як основне джерело
```

---

## 5. Реальний scope OLEG 4.0 Sister

Все, що треба на старті:

```text
1. Перевизначити піни.
2. Перевизначити драйвер прозорого OLED.
3. Перевизначити audio output / DAC / підсилювач path.
4. Адаптувати hardware config під HU-055 gift build.
```

Все інше лишається як є:

```text
✅ UI логіка
✅ Config Portal
✅ SpeakerConfig / NVS
✅ Weather / Clock / Player
✅ sleep kitty
✅ battery warning
✅ language logic
✅ reset/config behavior
```

---

## 6. Audio architecture

Не використовувати штатний Bluetooth/BT-тракт китайської балалайки як головне джерело.

Базовий audio path:

```text
ESP32
→ I²S
→ PCM5102A DAC
→ analog L/R
→ штатний analog input HU-055
→ штатний підсилювач / динаміки
```

Audio baseline з OLEG v3.5.x не ламати:

```text
defaultConfig()
set_mono_downmix(true)
set_volume(...)
```

Не повторювати старі погані audio-тести:

```text
BluetoothA2DPSinkQueued
set_raw_stream_reader
set_pin_config
I2S_LSB_FORMAT
I2S_STD_FORMAT
forced sample rate/channels/bits
NoVolumeControl
```

---

## 7. Transparent OLED target

Для OLEG 4.0 потрібно буде замінити display target на прозорий OLED.

Не вигадувати піни з памʼяті. У новому чаті попросити/взяти актуальні:

```text
- модель прозорого OLED
- інтерфейс: SPI/I2C
- драйвер: SSD1309 або інший
- DIN/MOSI
- CLK/SCK
- CS
- DC
- RST
- VCC рівень
- GND
```

Після цього робити перший hardware-target patch.

---

## 8. Arduino IDE warning

Нова робоча папка називається:

```text
OLEG_4_0_Sister_HU055
```

Arduino IDE любить правило:

```text
імʼя папки = імʼя .ino
```

Тому перший housekeeping-коміт може бути:

```bash
git mv Vitalik_Speaker_HARD_v3_2jingle_ui_cosmetic_LoLin32_MicroPython.ino OLEG_4_0_Sister_HU055.ino
git commit -m "Rename sketch for OLEG 4 Sister"
```

Або зробити це разом із першим hardware-target commit.

---

## 9. Remote note

Бо clone зроблено з локальної dev-репи, `origin` зараз вказує на локальний шлях.

Перевірити:

```bash
git remote -v
```

Якщо пізніше створюється окремий GitHub repo для OLEG 4.0 Sister:

```bash
git remote rename origin local-source
git remote add origin https://github.com/vitaliyrenkas-rgb/<NEW_REPO>.git
git push -u origin feature/oleg-4-sister-hu055
```

Поки GitHub repo не створено — можна працювати локально.

---

## 10. Stash note у старій dev-репі

У старій приватній dev-репі лишився stash:

```text
stash@{0}: On feature/v3.5-config-portal: public release cleanup local stash
```

Це НЕ в новій копії і не потрібно для OLEG 4.0.  
Не pop-ати без окремої потреби.

---

## 11. Перший старт у новому чаті

Команди для перевірки:

```bash
cd ~/Documents/Arduino/DIY_BT_Speaker_OLED_1_3_UI_v2/"OLED LolinESP32"/"OLEG_4_0_Sister_HU055"

git log --oneline --decorate -5
git status --short
git remote -v
```

Потім зробити архів для першого патча:

```bash
git archive --format=zip --output=src_oleg4_sister_start.zip HEAD src
```

Фраза для старту нового чату:

```text
Починаємо OLEG 4.0 Sister Edition. Окрема папка OLEG_4_0_Sister_HU055, гілка feature/oleg-4-sister-hu055, база 4b3114c Add low battery warning. Proof HU-055/PCM5102A не повторювати: точки L/R/GND і analog input уже відомі. Потрібно тільки перевизначити hardware target: прозорий OLED pins/driver і audio output під HU-055/PCM5102A. Решту логіки v3.5.x лишаємо як є.
```

Нормально будєт.

## 2026-07-18 — v4.0-sister-007 ADKEY UX fix

- BTN1 short: Player. BTN1 long: opens language selection, but does not auto-accept; release first, then BTN1=EN or BTN3=UA.
- BTN2 short: Clock. BTN2 long at boot or runtime: OLEG-SETUP config portal. Hold detection has dropout grace for the ADKEY ladder.
- BTN3 short: Weather. No long service action.
- ADKEY windows widened around measured HU-055 values: BTN1 ~0, BTN2 ~1805, BTN3 ~2860, idle ~4095.
