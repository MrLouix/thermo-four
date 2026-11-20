#include "settings.h"

#include <LittleFS.h>

Settings settings;

static const char *SETTINGS_PATH = "/settings.json";

void settingsInitDefaults() {
  settings = Settings();
  settings.probe[0].name = "Viande";
  settings.probe[0].alarm.highOn = true;
  settings.probe[0].alarm.high = 60;
  settings.probe[1].name = "Four";
  settings.probe[1].alarm.high = 220;
  settings.probe[1].alarm.low = 160;
  // Sujet ntfy aléatoire pour ne pas recevoir les notifications des autres.
  char topic[24];
  snprintf(topic, sizeof(topic), "thermo-%08lx", (unsigned long)esp_random());
  settings.ntfyTopic = topic;
}

bool Settings::hasWifi() const {
  for (const auto &w : wifi)
    if (w.ssid.length()) return true;
  return false;
}

void Settings::toJson(JsonObject o, bool withSecrets) const {
  JsonArray probes = o["probes"].to<JsonArray>();
  for (const auto &p : probe) {
    JsonObject j = probes.add<JsonObject>();
    j["name"] = p.name;
    j["rRef"] = p.rRef;
    j["a"] = p.sh.a;
    j["b"] = p.sh.b;
    j["c"] = p.sh.c;
    j["calibrated"] = p.calibrated;
    JsonObject a = j["alarm"].to<JsonObject>();
    a["highOn"] = p.alarm.highOn;
    a["high"] = p.alarm.high;
    a["lowOn"] = p.alarm.lowOn;
    a["low"] = p.alarm.low;
    a["preOn"] = p.alarm.preOn;
    a["preDelta"] = p.alarm.preDelta;
  }
  o["volume"] = volume;
  o["ntfyServer"] = ntfyServer;
  o["ntfyTopic"] = ntfyTopic;
  o["webhookUrl"] = webhookUrl;
  o["screenTimeoutS"] = screenTimeoutS;
  JsonArray w = o["wifi"].to<JsonArray>();
  for (const auto &c : wifi) {
    JsonObject j = w.add<JsonObject>();
    j["ssid"] = c.ssid;
    if (withSecrets) j["pass"] = c.pass;
  }
  if (withSecrets) o["apPass"] = apPass;
}

// N'écrase que les champs présents : l'interface peut envoyer un réglage seul.
void Settings::fromJson(JsonObjectConst o) {
  JsonArrayConst probes = o["probes"];
  for (size_t i = 0; i < probes.size() && i < hw::PROBE_COUNT; i++) {
    JsonObjectConst j = probes[i];
    ProbeSettings &p = probe[i];
    if (j["name"].is<const char *>()) p.name = j["name"].as<const char *>();
    if (j["rRef"].is<double>()) p.rRef = j["rRef"];
    if (j["a"].is<double>()) p.sh.a = j["a"];
    if (j["b"].is<double>()) p.sh.b = j["b"];
    if (j["c"].is<double>()) p.sh.c = j["c"];
    if (j["calibrated"].is<bool>()) p.calibrated = j["calibrated"];
    JsonObjectConst a = j["alarm"];
    if (a["highOn"].is<bool>()) p.alarm.highOn = a["highOn"];
    if (a["high"].is<float>()) p.alarm.high = a["high"];
    if (a["lowOn"].is<bool>()) p.alarm.lowOn = a["lowOn"];
    if (a["low"].is<float>()) p.alarm.low = a["low"];
    if (a["preOn"].is<bool>()) p.alarm.preOn = a["preOn"];
    if (a["preDelta"].is<float>()) p.alarm.preDelta = a["preDelta"];
  }
  if (o["volume"].is<uint8_t>()) {
    const uint8_t v = o["volume"].as<uint8_t>();
    volume = v > 3 ? 3 : v;
  }
  if (o["ntfyServer"].is<const char *>()) ntfyServer = o["ntfyServer"].as<const char *>();
  if (o["ntfyTopic"].is<const char *>()) ntfyTopic = o["ntfyTopic"].as<const char *>();
  if (o["webhookUrl"].is<const char *>()) webhookUrl = o["webhookUrl"].as<const char *>();
  if (o["screenTimeoutS"].is<uint16_t>()) screenTimeoutS = o["screenTimeoutS"];
  JsonArrayConst w = o["wifi"];
  for (size_t i = 0; i < w.size() && i < MAX_WIFI; i++) {
    JsonObjectConst j = w[i];
    if (j["ssid"].is<const char *>()) wifi[i].ssid = j["ssid"].as<const char *>();
    // Mot de passe absent = inchangé (l'API ne renvoie jamais les mots de passe).
    if (j["pass"].is<const char *>()) wifi[i].pass = j["pass"].as<const char *>();
  }
  if (o["apPass"].is<const char *>()) {
    String p = o["apPass"].as<const char *>();
    if (p.length() >= 8) apPass = p;  // minimum WPA2
  }
}

bool settingsLoad() {
  settingsInitDefaults();
  File f = LittleFS.open(SETTINGS_PATH, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  settings.fromJson(doc.as<JsonObjectConst>());
  return true;
}

bool settingsSave() {
  JsonDocument doc;
  settings.toJson(doc.to<JsonObject>(), true);
  File f = LittleFS.open(SETTINGS_PATH, "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}
