// État courant partagé entre les modules (lu par l'affichage et le web).
#pragma once
#include <Arduino.h>

#include "config.h"
#include "thermistor.h"

struct ProbeReading {
  thermo::ProbeStatus status = thermo::ProbeStatus::Open;
  double resistance = NAN;
  float tempC = NAN;
  bool valid() const { return status == thermo::ProbeStatus::Ok; }
};

struct TimerState {
  bool running = false;
  uint32_t endMs = 0;
  uint32_t durationS = 0;
  uint32_t remainingS() const {
    if (!running) return 0;
    int32_t r = (int32_t)(endMs - millis());
    return r > 0 ? (uint32_t)(r + 999) / 1000 : 0;
  }
};

struct AppState {
  ProbeReading probe[hw::PROBE_COUNT];
  uint8_t shownProbe = 0;        // sonde affichée en grand et réglée aux boutons
  int batteryPct = -1;           // -1 = jauge absente
  float batteryV = NAN;
  bool alarmRinging = false;     // buzzer en cours, à acquitter
  TimerState timer;
  bool showIp = false;
  uint32_t showIpUntil = 0;
};

extern AppState app;
