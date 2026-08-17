# SW1 / GPIO27 bench test

## Wiring under test

```text
switched MH-M18 VCC -- 91k --+-- GPIO27
                              |
                            120k
                              |
                         Lolita GND
```

All measurements are relative to Lolita GND.

## Before connecting GPIO27

Measure the divider midpoint with it disconnected from GPIO27:

| SW1 | Expected midpoint |
|---|---:|
| OFF | approximately 0 V |
| ON, switched rail 4.6 V | approximately 2.62 V |
| ON, switched rail 5.0 V | approximately 2.84 V |
| ON, switched rail 5.2 V | approximately 2.96 V |

Connect the midpoint to GPIO27 only after these levels are confirmed.

## Firmware smoke test

1. Boot with SW1 ON: normal greeting, Wi-Fi/weather cycle and Bluetooth startup.
2. Switch OFF during normal runtime: after 500 ms the OLED turns off and Serial prints `SW1 OFF: OLED off, entering deep sleep`.
3. Switch ON again: GPIO27 HIGH wakes the ESP32 and a normal boot starts.
4. Reset or apply battery power while SW1 is already OFF: OLED, Wi-Fi, Bluetooth and I2S must not start; switching ON must wake the ESP32.
5. Repeat OFF/ON at least ten times, including while the Config Portal is active.

## Pass criteria

- No false sleep while SW1 is ON.
- OLED always turns off after SW1 OFF.
- Every SW1 ON wakes into a normal boot without pressing RESET.
- Divider midpoint never exceeds 3.3 V in either battery or USB operation.

Status before physical test: **REFERENCE-BASED / UNVALIDATED ON BENCH**.
