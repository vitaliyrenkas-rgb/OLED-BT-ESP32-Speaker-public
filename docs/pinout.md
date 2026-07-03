# OLED BT Speaker Pinout

Reference wiring used by the current ESP32/LoLin build.

## OLED 128×64 I2C

| Signal | GPIO |
|---|---:|
| SDA | 23 |
| SCL | 22 |

## MAX98357A I2S amplifier

| Signal | GPIO |
|---|---:|
| BCLK | 26 |
| LRC / WS | 25 |
| DIN | 32 |

Use a common ground between ESP32, amplifier, OLED, battery/charger, and controls.

## Buttons

Three front buttons are used for Player / Clock / Weather navigation. The current firmware also uses button combinations for language/config flows.

## Battery and power sensing

| Function | GPIO | Notes |
|---|---:|---|
| Battery voltage | 34 | 100k/100k divider in the reference build |
| USB/VBUS detect | 33 | 100k/100k divider in the reference build |
| Volume pot wiper | 35 | Pot ends to 3.3V and GND |

GPIO35 must never see more than 3.3V.
