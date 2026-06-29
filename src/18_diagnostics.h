// Auto-split from monolithic OLEG sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= DIAGNOSTICS =================
void logHeapDiagnostics() {
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" | Min free heap: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.print(" | Screen: ");
  Serial.print(currentScreen);
  Serial.print(" | BT: ");
  Serial.print(btConnected ? "ON" : "OFF");
  Serial.print(" | Bat: ");
  Serial.print(batteryVoltage, 2);
  Serial.print("V/");
  Serial.print(batteryPercent);
  Serial.print("%");
  Serial.print(batteryCharging ? "/CHG" : "/BAT");
  Serial.print(" | WiFiSync: ");
  Serial.println(wifiLastSyncOk ? "OK" : "FAIL");
}

void hardResetConfigAndRestart() {
  Serial.println("CONFIG RESET: hard erase NVS");
  prefs.clear();
  prefs.end();

  // Uploading firmware does NOT erase Preferences/NVS.
  // This hard-erases NVS to force language/config selection again.
  nvs_flash_erase();
  nvs_flash_init();

  delay(800);
  ESP.restart();
}
