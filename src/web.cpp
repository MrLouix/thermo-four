#include "web.h"

#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "buzzer.h"
#include "config.h"
#include "controller.h"
#include "logger.h"
#include "net.h"
#include "settings.h"

namespace {

AsyncWebServer server(80);
AsyncEventSource events("/events");
uint32_t lastSse = 0;
volatile bool restartPending = false;
uint32_t restartAt = 0;

void sendJson(AsyncWebServerRequest *req, JsonDocument &doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  req->send(code, "application/json", out);
}

void sendResult(AsyncWebServerRequest *req, const char *err) {
  JsonDocument doc;
  doc["ok"] = err == nullptr;
  if (err) doc["error"] = err;
  sendJson(req, doc, err ? 400 : 200);
}

// Enregistre un gestionnaire POST avec corps JSON.
void onJson(const char *path, ArJsonRequestHandlerFunction fn) {
  auto *h = new AsyncCallbackJsonWebHandler(path, fn);
  h->setMethod(HTTP_POST);
  server.addHandler(h);
}

}  // namespace

void webBegin() {
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *req) {
    JsonDocument doc;
    stateJson(doc.to<JsonObject>());
    sendJson(req, doc);
  });

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *req) {
    JsonDocument doc;
    settings.toJson(doc.to<JsonObject>(), false);  // jamais les mots de passe
    sendJson(req, doc);
  });

  onJson("/api/settings", [](AsyncWebServerRequest *req, JsonVariant &json) {
    const bool wifiChanged = json["wifi"].is<JsonArrayConst>() || json["apPass"].is<const char *>();
    settings.fromJson(json.as<JsonObjectConst>());
    settingsSave();
    sendResult(req, nullptr);
    if (wifiChanged) {  // redémarrer pour appliquer les réseaux
      restartPending = true;
      restartAt = millis() + 1500;
    }
  });

  onJson("/api/timer", [](AsyncWebServerRequest *req, JsonVariant &json) {
    timerStart(json["seconds"] | 0);
    sendResult(req, nullptr);
  });

  onJson("/api/ack", [](AsyncWebServerRequest *req, JsonVariant &) {
    acknowledgeAlarm();
    sendResult(req, nullptr);
  });

  onJson("/api/buzzer/test", [](AsyncWebServerRequest *req, JsonVariant &json) {
    if (json["volume"].is<uint8_t>()) settings.volume = json["volume"].as<uint8_t>() & 3;
    buzzerPlay(Sound::Test);
    sendResult(req, nullptr);
  });

  onJson("/api/record", [](AsyncWebServerRequest *req, JsonVariant &json) {
    if (json["stop"] | false) {
      loggerStop();
      return sendResult(req, nullptr);
    }
    const time_t at = json["startAt"] | 0L;
    sendResult(req, loggerSchedule(at, json["durationS"] | 3600UL, json["intervalS"] | 10));
  });

  server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *req) {
    JsonDocument doc;
    loggerList(doc.to<JsonArray>());
    sendJson(req, doc);
  });

  onJson("/api/logs/delete", [](AsyncWebServerRequest *req, JsonVariant &json) {
    const String name = json["name"] | "";
    sendResult(req, loggerDelete(name) ? nullptr : "Suppression impossible");
  });

  server.on("/api/calibrate", HTTP_GET, [](AsyncWebServerRequest *req) {
    const uint8_t probe = req->hasParam("probe") ? req->getParam("probe")->value().toInt() : 0;
    JsonDocument doc;
    calibJson(probe, doc.to<JsonObject>());
    sendJson(req, doc);
  });

  onJson("/api/calibrate", [](AsyncWebServerRequest *req, JsonVariant &json) {
    const uint8_t probe = json["probe"] | 0;
    const String action = json["action"] | "";
    if (action == "capture")
      return sendResult(req, calibCapture(probe, json["point"] | 0, json["tempC"] | 0.0f));
    if (action == "apply") return sendResult(req, calibApply(probe));
    if (action == "beta")
      return sendResult(req, calibFromBeta(probe, json["r25"] | 0.0, json["beta"] | 0.0));
    sendResult(req, "Action inconnue");
  });

  server.addHandler(&events);
  server.serveStatic("/logs/", LittleFS, "/logs/");
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
  server.onNotFound([](AsyncWebServerRequest *req) {
    // En mode portail, toute URL ramène à la page de configuration.
    if (netPortalActive()) return req->redirect("/#reglages");
    req->send(404, "text/plain", "Introuvable");
  });
  server.begin();
}

void webLoop() {
  const uint32_t now = millis();
  if (restartPending && (int32_t)(now - restartAt) > 0) ESP.restart();
  if (now - lastSse < timing::SSE_MS || events.count() == 0) return;
  lastSse = now;
  JsonDocument doc;
  stateJson(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  events.send(out.c_str(), "state", now);
}
