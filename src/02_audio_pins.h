// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= MAX98357A / I2S =================
// OLED uses GPIO22 for SCL, so I2S DOUT is moved to GPIO32.
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 32
