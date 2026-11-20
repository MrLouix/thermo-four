#include "controller.h"

#include "alarm.h"
#include "buzzer.h"
#include "logger.h"
#include "net.h"
#include "notify.h"
#include "power.h"
#include "probes.h"
#include "settings.h"
#include "state.h"

AppState app;

namespace {

thermo::AlarmChannel channels[hw::PROBE_COUNT];
bool batteryLowSent = false;
uint32_t lastBatteryMs = 0;

struct CalPoint {
  bool set = false;
  float tempC = 0;
  double r = 0;
};
CalPoint calib[hw::PROBE_COUNT][3];

void ring(Sound s) {
  buzzerPlay(s);
  if (s == Sound::Target || s == Sound::Timer) app.alarmRinging = true;
}

void handleEvents(uint8_t i, uint8_t ev) {
  if (ev == thermo::EV_NONE) return;
  const ProbeSettings &p = settings.probe[i];
  const float t = app.probe[i].tempC;
  char title[48], body[160];
  if (ev & thermo::EV_HIGH) {
    snprintf(title, sizeof(title), "%s : consigne atteinte", p.name.c_str());
    snprintf(body, sizeof(body), "%.1f °C (consigne %.0f °C)", t, p.alarm.high);
    notify(title, body, NotifyPrio::Urgent, "fire");
    ring(Sound::Target);
    app.shownProbe = i;
  }
  if (ev & thermo::EV_PRE) {
    snprintf(title, sizeof(title), "%s : bientôt prêt", p.name.c_str());
    snprintf(body, sizeof(body), "%.1f °C, consigne %.0f °C", t, p.alarm.high);
    notify(title, body, NotifyPrio::High, "hourglass");
    ring(Sound::PreAlert);
  }
  if (ev & thermo::EV_LOW) {
    snprintf(title, sizeof(title), "%s : sous le seuil bas", p.name.c_str());
    snprintf(body, sizeof(body), "%.1f °C (seuil %.0f °C)", t, p.alarm.low);
    notify(title, body, NotifyPrio::High, "snowflake");
    ring(Sound::Low);
  }
  if (ev & thermo::EV_FAULT) {
    snprintf(title, sizeof(title), "%s : sonde en défaut", p.name.c_str());
    snprintf(body, sizeof(body), "Sonde débranchée ou en court-circuit");
    notify(title, body, NotifyPrio::High, "warning");
    ring(Sound::Fault);
  }
  if (ev & thermo::EV_FAULT_CLEARED) {
    snprintf(title, sizeof(title), "%s : sonde reconnectée", p.name.c_str());
    notify(title, "", NotifyPrio::Low, "white_check_mark");
  }
}

}  // namespace

void controllerBegin() {
  powerUpdate();
  lastBatteryMs = millis();
}

void controllerTick() {
  probesMeasure();
  const bool recording = loggerActive();
  for (uint8_t i = 0; i < hw::PROBE_COUNT; i++) {
    const auto &a = settings.probe[i].alarm;
    const bool watch = recording || a.highOn || a.lowOn;
    handleEvents(i, channels[i].update(a, app.probe[i].valid(), app.probe[i].tempC, watch));
  }
}

void controllerLoop() {
  const uint32_t now = millis();
  if (app.timer.running && app.timer.remainingS() == 0) {
    app.timer.running = false;
    notify("Minuteur terminé", "Le compte à rebours est écoulé", NotifyPrio::Urgent, "alarm_clock");
    ring(Sound::Timer);
  }
  if (now - lastBatteryMs > 30000) {
    lastBatteryMs = now;
    powerUpdate();
    if (app.batteryPct >= 0) {
      if (!batteryLowSent && app.batteryPct <= BATTERY_LOW_PCT) {
        batteryLowSent = true;
        char body[32];
        snprintf(body, sizeof(body), "%d %%", app.batteryPct);
        notify("Batterie faible", body, NotifyPrio::High, "battery");
        buzzerPlay(Sound::Battery);
      } else if (app.batteryPct > BATTERY_LOW_PCT + 5) {
        batteryLowSent = false;
      }
    }
  }
  if (app.showIp && (int32_t)(now - app.showIpUntil) > 0) app.showIp = false;
}

void acknowledgeAlarm() {
  app.alarmRinging = false;
  buzzerStop();
}

void adjustTarget(uint8_t i, float delta) {
  if (i >= hw::PROBE_COUNT) return;
  auto &a = settings.probe[i].alarm;
  a.highOn = true;
  a.high = constrain(a.high + delta, 0.0f, 300.0f);
  // Sauvegarde différée : voir main.cpp (évite d'user la flash à chaque clic).
}

void timerStart(uint32_t s) {
  if (s == 0) return timerStop();
  app.timer.durationS = s;
  app.timer.endMs = millis() + s * 1000UL;
  app.timer.running = true;
}

void timerStop() { app.timer.running = false; }

void stateJson(JsonObject o) {
  JsonArray probes = o["probes"].to<JsonArray>();
  for (uint8_t i = 0; i < hw::PROBE_COUNT; i++) {
    JsonObject j = probes.add<JsonObject>();
    const ProbeReading &r = app.probe[i];
    j["name"] = settings.probe[i].name;
    j["status"] = r.valid() ? "ok" : (r.status == thermo::ProbeStatus::Open ? "absente" : "court-circuit");
    if (r.valid()) {
      j["t"] = serialized(String(r.tempC, 1));
      j["r"] = (long)r.resistance;
    }
    j["highActive"] = channels[i].highActive();
    j["lowActive"] = channels[i].lowActive();
  }
  o["shown"] = app.shownProbe;
  o["battery"] = app.batteryPct;
  o["alarm"] = app.alarmRinging;
  o["timerS"] = app.timer.remainingS();
  o["time"] = netTimeValid() ? (long)time(nullptr) : 0;
  o["ip"] = netIp();
  loggerStatus(o["record"].to<JsonObject>());
}

const char *calibCapture(uint8_t probe, uint8_t point, float tempC) {
  if (probe >= hw::PROBE_COUNT || point > 2) return "Paramètres invalides";
  const double r = probeResistance(probe);
  if (isnan(r)) return "Sonde absente ou en défaut";
  calib[probe][point] = {true, tempC, r};
  return nullptr;
}

const char *calibApply(uint8_t probe) {
  if (probe >= hw::PROBE_COUNT) return "Sonde invalide";
  auto &c = calib[probe];
  if (!c[0].set || !c[1].set || !c[2].set) return "Il faut les 3 points";
  thermo::ShCoeffs k;
  if (!thermo::fromThreePoints(c[0].tempC, c[0].r, c[1].tempC, c[1].r, c[2].tempC, c[2].r, k))
    return "Points incohérents (températures trop proches ?)";
  settings.probe[probe].sh = k;
  settings.probe[probe].calibrated = true;
  settingsSave();
  return nullptr;
}

const char *calibFromBeta(uint8_t probe, double r25, double beta) {
  if (probe >= hw::PROBE_COUNT || r25 <= 0 || beta <= 0) return "Paramètres invalides";
  settings.probe[probe].sh = thermo::fromBeta(r25, beta);
  settings.probe[probe].calibrated = false;
  settingsSave();
  return nullptr;
}

void calibJson(uint8_t probe, JsonObject o) {
  if (probe >= hw::PROBE_COUNT) return;
  JsonArray pts = o["points"].to<JsonArray>();
  for (const auto &c : calib[probe]) {
    JsonObject j = pts.add<JsonObject>();
    j["set"] = c.set;
    if (c.set) {
      j["t"] = c.tempC;
      j["r"] = (long)c.r;
    }
  }
  o["calibrated"] = settings.probe[probe].calibrated;
}
