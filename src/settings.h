// Réglages persistants (LittleFS, /settings.json).
#pragma once
#include <ArduinoJson.h>

#include "alarm.h"
#include "config.h"
#include "thermistor.h"

struct WifiCred {
  String ssid;
  String pass;
};

struct ProbeSettings {
  String name;
  double rRef = hw::DEFAULT_RREF;
  thermo::ShCoeffs sh = thermo::fromBeta(hw::DEFAULT_R25, hw::DEFAULT_BETA);
  bool calibrated = false;
  thermo::AlarmConfig alarm;
};

struct Settings {
  ProbeSettings probe[hw::PROBE_COUNT];
  uint8_t volume = 3;            // 0 = coupé, 1 = faible, 2 = moyen, 3 = fort
  String ntfyServer = "https://ntfy.sh";
  String ntfyTopic;              // vide = notifications désactivées
  String webhookUrl;             // optionnel (n8n…)
  WifiCred wifi[MAX_WIFI];
  String apPass = AP_DEFAULT_PASS;
  uint16_t screenTimeoutS = 60;

  bool hasWifi() const;
  void toJson(JsonObject o, bool withSecrets) const;
  void fromJson(JsonObjectConst o);
};

extern Settings settings;

void settingsInitDefaults();
bool settingsLoad();
bool settingsSave();
