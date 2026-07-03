// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= STARTUP JINGLE =================
// void playJingleTone(float freq, int durationMs, float volume = 0.22f) {
//   const int sampleRate = 44100;
//   const int samples = (sampleRate * durationMs) / 1000;
//   const int chunk = 128;

//   int16_t buffer[chunk * 2]; // stereo: L/R
//   size_t bytesWritten = 0;

//   for (int i = 0; i < samples; i += chunk) {
//     int count = min(chunk, samples - i);

//     for (int j = 0; j < count; j++) {
//       int idx = i + j;
//       float t = (float)idx / sampleRate;

//       // Small fade in/out to avoid clicks.
//       float fade = 1.0f;
//       int fadeSamples = min(300, samples / 8);
//       if (idx < fadeSamples) fade = (float)idx / fadeSamples;
//       else if (idx > samples - fadeSamples) fade = (float)(samples - idx) / fadeSamples;

//       int16_t sample = (int16_t)(sin(2.0f * PI * freq * t) * 32767.0f * volume * fade);

//       buffer[j * 2 + 0] = sample;
//       buffer[j * 2 + 1] = sample;
//     }

//     i2s_write(I2S_NUM_0, buffer, count * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
//   }
// }

// void playJingleSilence(int durationMs) {
//   const int sampleRate = 44100;
//   const int samples = (sampleRate * durationMs) / 1000;
//   const int chunk = 128;

//   int16_t buffer[chunk * 2] = {0};
//   size_t bytesWritten = 0;

//   for (int i = 0; i < samples; i += chunk) {
//     int count = min(chunk, samples - i);
//     i2s_write(I2S_NUM_0, buffer, count * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
//   }
// }

// void playStartupJingle() {
//   // FIX v3.2:
//   // Short hi-fi / early-MP3-car-head-unit style startup sound.
//   // Plays before Bluetooth A2DP starts, so it does not fight the BT audio stream.

//   i2s_config_t jingle_i2s_config = {
//     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
//     .sample_rate = 44100,
//     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
//     .communication_format = I2S_COMM_FORMAT_STAND_I2S,
//     .intr_alloc_flags = 0,
//     .dma_buf_count = 8,
//     .dma_buf_len = 64,
//     .use_apll = false,
//     .tx_desc_auto_clear = true,
//     .fixed_mclk = 0
//   };

//   i2s_pin_config_t jingle_pin_config = {
//     .bck_io_num = I2S_BCLK,
//     .ws_io_num = I2S_LRC,
//     .data_out_num = I2S_DOUT,
//     .data_in_num = I2S_PIN_NO_CHANGE
//   };

//   i2s_driver_install(I2S_NUM_0, &jingle_i2s_config, 0, NULL);
//   i2s_set_pin(I2S_NUM_0, &jingle_pin_config);
//   i2s_zero_dma_buffer(I2S_NUM_0);

//   // Selected "hifi" jingle: C5 -> E5 -> A5.
//   playJingleTone(523.25f, 100);
//   playJingleSilence(20);
//   playJingleTone(659.25f, 100);
//   playJingleSilence(20);
//   playJingleTone(880.00f, 240);

//   playJingleSilence(80);
//   i2s_driver_uninstall(I2S_NUM_0);
// }
