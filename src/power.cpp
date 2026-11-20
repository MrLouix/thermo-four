#include "power.h"

#include <Adafruit_MAX1704X.h>
#include <driver/rtc_io.h>

#include "config.h"
#include "display.h"
#include "state.h"

static Adafruit_MAX17048 gauge;
static bool gaugeOk = false;

void powerBegin() { gaugeOk = gauge.begin(&Wire); }

void powerUpdate() {
  if (!gaugeOk) {
    app.batteryPct = -1;
    return;
  }
  const float pct = gauge.cellPercent();
  app.batteryV = gauge.cellVoltage();
  app.batteryPct = pct < 0 ? 0 : (pct > 100 ? 100 : (int)(pct + 0.5f));
}

void powerOff() {
  displayInstance().sleep(true);
  digitalWrite(pins::PROBE_EXC, LOW);
  // Attendre le relâchement du bouton, sinon réveil immédiat.
  while (digitalRead(pins::BTN_MODE) == LOW) delay(10);
  delay(50);
  // Le rappel interne doit rester actif pendant la veille profonde.
  rtc_gpio_pullup_en((gpio_num_t)pins::BTN_MODE);
  rtc_gpio_pulldown_dis((gpio_num_t)pins::BTN_MODE);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)pins::BTN_MODE, 0);
  esp_deep_sleep_start();
}
