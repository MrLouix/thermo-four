#include <U8g2lib.h>

#include "display.h"

namespace {

class OledDisplay : public Display {
 public:
  bool begin() override {
    u8g2_.setI2CAddress(0x3C << 1);
    const bool ok = u8g2_.begin();
    u8g2_.enableUTF8Print();
    return ok;
  }

  void sleep(bool off) override { u8g2_.setPowerSave(off ? 1 : 0); }

  void render(const UiModel &m) override {
    u8g2_.clearBuffer();

    if (m.apInfo.length()) {
      u8g2_.setFont(u8g2_font_6x10_tf);
      u8g2_.drawStr(0, 10, "Configuration Wi-Fi");
      u8g2_.setCursor(0, 26);
      u8g2_.print(m.apInfo);
      u8g2_.drawStr(0, 42, "http://192.168.4.1");
      u8g2_.sendBuffer();
      return;
    }

    // Barre d'état
    u8g2_.setFont(u8g2_font_5x8_tf);
    u8g2_.setCursor(0, 7);
    u8g2_.print(m.wifiOk ? "WiFi" : "----");
    if (m.recording) u8g2_.drawStr(30, 7, "REC");
    if (m.timerS) {
      char t[12];
      snprintf(t, sizeof(t), "%lu:%02lu", (unsigned long)(m.timerS / 60), (unsigned long)(m.timerS % 60));
      u8g2_.drawStr(52, 7, t);
    }
    if (m.batteryPct >= 0) {
      char b[6];
      snprintf(b, sizeof(b), "%d%%", m.batteryPct);
      u8g2_.drawStr(108, 7, b);
    }

    if (m.showIp) {
      u8g2_.setFont(u8g2_font_6x10_tf);
      u8g2_.drawStr(0, 30, "Adresse :");
      u8g2_.setCursor(0, 46);
      u8g2_.print(m.ip);
      u8g2_.drawStr(0, 60, "thermo.local");
      u8g2_.sendBuffer();
      return;
    }

    const uint8_t s = m.shown;
    const uint8_t o = 1 - s;

    // Sonde affichée en grand
    u8g2_.setFont(u8g2_font_6x10_tf);
    u8g2_.setCursor(0, 19);
    u8g2_.print(m.probeName[s]);
    u8g2_.setFont(u8g2_font_logisoso24_tf);
    char big[12];
    if (m.valid[s]) snprintf(big, sizeof(big), "%.1f", m.temp[s]);
    else snprintf(big, sizeof(big), "---");
    u8g2_.drawStr(0, 46, big);
    const int bigWidth = u8g2_.getStrWidth(big);
    u8g2_.setFont(u8g2_font_6x10_tf);
    u8g2_.drawUTF8(bigWidth + 3, 30, "°C");

    // Consigne
    if (!isnan(m.target)) {
      char t[16];
      snprintf(t, sizeof(t), "→%.0f°", m.target);
      u8g2_.drawUTF8(92, 46, t);
    }

    // Seconde sonde en petit
    u8g2_.setCursor(0, 62);
    u8g2_.print(m.probeName[o]);
    u8g2_.print(" ");
    if (m.valid[o]) u8g2_.printf("%.0f", m.temp[o]);
    else u8g2_.print("---");
    u8g2_.print("°C");

    if (m.alarm && (millis() / 400) % 2) u8g2_.drawFrame(0, 9, 128, 55);
    u8g2_.sendBuffer();
  }

 private:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_{U8G2_R0, U8X8_PIN_NONE};
};

}  // namespace

Display &displayInstance() {
  static OledDisplay d;
  return d;
}
