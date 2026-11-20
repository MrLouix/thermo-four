#include "logger.h"

#include <LittleFS.h>

#include <algorithm>
#include <vector>

#include "net.h"
#include "notify.h"
#include "state.h"

namespace {

const char *DIR = "/logs";

bool pending = false;
bool active = false;
time_t startAt = 0;
uint32_t durationS = 0;
uint16_t intervalS = 10;
uint32_t startedMs = 0;
uint32_t lastSampleMs = 0;
String currentName;
File file;

void pruneOld() {
  std::vector<String> names;
  File dir = LittleFS.open(DIR);
  if (!dir) return;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) names.push_back(f.name());
  std::sort(names.begin(), names.end());
  while (names.size() >= MAX_LOG_FILES) {
    LittleFS.remove(String(DIR) + "/" + names.front());
    names.erase(names.begin());
  }
}

void openFile() {
  if (!LittleFS.exists(DIR)) LittleFS.mkdir(DIR);
  pruneOld();
  char name[32];
  if (netTimeValid()) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    strftime(name, sizeof(name), "%Y%m%d-%H%M%S.csv", &t);
  } else {
    snprintf(name, sizeof(name), "sans-heure-%lu.csv", (unsigned long)(millis() / 1000));
  }
  currentName = name;
  file = LittleFS.open(String(DIR) + "/" + currentName, "w");
  if (file) file.println("horodatage,secondes,sonde1_c,sonde2_c,batterie_pct");
  active = (bool)file;
  pending = false;
  startedMs = millis();
  lastSampleMs = startedMs - (uint32_t)intervalS * 1000;  // échantillon immédiat
  if (active) notify("Enregistrement démarré", currentName.c_str(), NotifyPrio::Low, "red_circle");
}

void writeSample() {
  const uint32_t elapsed = (millis() - startedMs) / 1000;
  char ts[24] = "";
  if (netTimeValid()) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", &t);
  }
  char line[96];
  char t1[10] = "", t2[10] = "";
  if (app.probe[0].valid()) snprintf(t1, sizeof(t1), "%.1f", app.probe[0].tempC);
  if (app.probe[1].valid()) snprintf(t2, sizeof(t2), "%.1f", app.probe[1].tempC);
  snprintf(line, sizeof(line), "%s,%lu,%s,%s,%d", ts, (unsigned long)elapsed, t1, t2, app.batteryPct);
  file.println(line);
  file.flush();  // garder les données en cas de coupure
}

}  // namespace

const char *loggerSchedule(time_t at, uint32_t duration, uint16_t interval) {
  if (active || pending) return "Un enregistrement est déjà en cours ou programmé";
  if (duration < 10 || duration > 48UL * 3600) return "Durée hors limites (10 s à 48 h)";
  if (interval < 1 || interval > 3600) return "Intervalle hors limites (1 à 3600 s)";
  if (at != 0) {
    if (!netTimeValid()) return "Heure inconnue (pas de NTP) : seul le démarrage immédiat est possible";
    if (at <= time(nullptr)) at = 0;
  }
  startAt = at;
  durationS = duration;
  intervalS = interval;
  if (at == 0) openFile();
  else pending = true;
  return (at == 0 && !active) ? "Impossible de créer le fichier" : nullptr;
}

void loggerStop() {
  if (active) {
    file.close();
    notify("Enregistrement terminé", currentName.c_str(), NotifyPrio::Low, "white_check_mark");
  }
  active = pending = false;
}

bool loggerActive() { return active; }
bool loggerPending() { return pending; }

void loggerLoop() {
  if (pending && netTimeValid() && time(nullptr) >= startAt) openFile();
  if (!active) return;
  const uint32_t now = millis();
  if (now - startedMs >= durationS * 1000UL) {
    loggerStop();
    return;
  }
  if (now - lastSampleMs >= (uint32_t)intervalS * 1000) {
    lastSampleMs = now;
    writeSample();
  }
}

void loggerStatus(JsonObject o) {
  o["active"] = active;
  o["pending"] = pending;
  o["file"] = currentName;
  o["startAt"] = (long)startAt;
  o["durationS"] = durationS;
  o["intervalS"] = intervalS;
  o["elapsedS"] = active ? (millis() - startedMs) / 1000 : 0;
}

void loggerList(JsonArray a) {
  File dir = LittleFS.open(DIR);
  if (!dir) return;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    JsonObject j = a.add<JsonObject>();
    j["name"] = f.name();
    j["size"] = f.size();
  }
}

bool loggerDelete(const String &name) {
  if (name.indexOf('/') >= 0 || name.indexOf("..") >= 0) return false;
  if (active && name == currentName) return false;
  return LittleFS.remove(String(DIR) + "/" + name);
}
