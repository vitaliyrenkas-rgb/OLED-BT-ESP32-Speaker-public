// Auto-split from monolithic OLED BT speaker sketch.
// Keep behavioral changes out of this structural split unless explicitly noted.

// ================= TEXT HELPERS =================
int textW(const String &s) {
  return u8g2.getUTF8Width(s.c_str());
}

void requestRedraw() {
  forceRedraw = true;
}

void centerTextSafe(const String &s, int y, const uint8_t *font) {
  u8g2.setFont(font);
  int w = u8g2.getUTF8Width(s.c_str());
  int x = (128 - w) / 2;
  if (x < 0) x = 0;
  u8g2.setCursor(x, y);
  u8g2.print(s);
}

void centerText(const String &s, int y, const uint8_t *font) {
  centerTextSafe(s, y, font);
}

bool isTimeValid() {
  struct tm t;
  if (!getLocalTime(&t, 5)) return false;
  return t.tm_year >= (2024 - 1900);
}

String timeStr() {
  struct tm t;
  if (!getLocalTime(&t, 5) || t.tm_year < (2024 - 1900)) return "--:--";

  char buf[6];
  sprintf(buf, "%02d:%02d", t.tm_hour, t.tm_min);
  return String(buf);
}

String dateStr() {
  struct tm t;
  if (!getLocalTime(&t, 5) || t.tm_year < (2024 - 1900)) {
    return uiLang == LANG_UA ? "час не синхр." : "time not sync";
  }

  if (uiLang == LANG_UA) {
    const char* days[] = { "Нд", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб" };
    const char* months[] = {
      "січ.", "лют.", "бер.", "квіт.", "трав.", "черв.",
      "лип.", "серп.", "вер.", "жовт.", "лист.", "груд."
    };
    return String(days[t.tm_wday]) + ", " + String(t.tm_mday) + " " + String(months[t.tm_mon]);
  } else {
    const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    const char* months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return String(days[t.tm_wday]) + ", " + String(t.tm_mday) + " " + String(months[t.tm_mon]);
  }
}
