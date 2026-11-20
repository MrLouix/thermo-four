#include "probes.h"

#include <Adafruit_ADS1X15.h>

#include "settings.h"

static Adafruit_ADS1115 ads;
static bool adsOk = false;

bool probesBegin() {
  pinMode(pins::PROBE_EXC, OUTPUT);
  digitalWrite(pins::PROBE_EXC, LOW);
  adsOk = ads.begin(hw::ADS_ADDR, &Wire);
  if (adsOk) {
    ads.setGain(GAIN_ONE);  // ±4,096 V
    ads.setDataRate(RATE_ADS1115_128SPS);
  }
  return adsOk;
}

// Moyenne de quelques lectures pour lisser le bruit.
static int32_t readAvg(uint8_t ch, uint8_t n = 4) {
  int32_t sum = 0;
  for (uint8_t i = 0; i < n; i++) sum += ads.readADC_SingleEnded(ch);
  return sum / n;
}

void probesMeasure() {
  if (!adsOk) {
    for (auto &p : app.probe) p = ProbeReading();
    return;
  }
  digitalWrite(pins::PROBE_EXC, HIGH);
  delay(2);  // stabilisation (condensateurs de 100 nF)
  const int32_t exc = readAvg(hw::ADS_CH_EXC);
  int32_t raw[hw::PROBE_COUNT];
  for (uint8_t i = 0; i < hw::PROBE_COUNT; i++) raw[i] = readAvg(hw::ADS_CH_PROBE[i]);
  digitalWrite(pins::PROBE_EXC, LOW);

  for (uint8_t i = 0; i < hw::PROBE_COUNT; i++) {
    ProbeReading &r = app.probe[i];
    const double ratio = exc > 100 ? (double)raw[i] / (double)exc : 0.0;
    r.status = thermo::statusFromRatio(ratio);
    if (r.valid()) {
      r.resistance = thermo::resistanceFromRatio(ratio, settings.probe[i].rRef);
      r.tempC = (float)thermo::temperatureC(r.resistance, settings.probe[i].sh);
    } else {
      r.resistance = NAN;
      r.tempC = NAN;
    }
  }
}

double probeResistance(uint8_t i) {
  if (i >= hw::PROBE_COUNT || !app.probe[i].valid()) return NAN;
  return app.probe[i].resistance;
}
