# OLEG LoLin Pinout

Parsed from the uploaded sketch.

## OLED I2C

| Signal | GPIO |
|---|---:|
| OLED SDA | 23 |
| OLED SCL | 22 |

## MAX98357A / I2S

| Signal | GPIO |
|---|---:|
| BCLK | 26 |
| LRC / WS | 25 |
| DIN / DATA | 32 |

## Buttons

| Function | GPIO |
|---|---:|
| Player / Button 1 | 13 |
| Clock / Button 2 | 16 |
| Weather / Button 3 | 17 |
| Config reset pin define | 35 |
| Battery ADC | 34 |

## Notes

Config reset flow is currently handled by the button combo:
`BTN_PLAYER + BTN_WEATHER` held for 5 seconds.

Battery ADC may float on LoLin until the physical battery divider is wired.
