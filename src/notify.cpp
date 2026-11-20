#include "notify.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "settings.h"

namespace {

struct Msg {
  char title[48];
  char body[160];
  char tags[32];
  uint8_t prio;
};

QueueHandle_t queue = nullptr;

bool post(const String &url, const char *contentType, const String &payload) {
  HTTPClient http;
  WiFiClientSecure secure;
  WiFiClient plain;
  bool ok;
  if (url.startsWith("https://")) {
    // v1 : pas de vérification du certificat (pas de magasin de CA embarqué).
    // À durcir en v2 avec le certificat racine du serveur ntfy.
    secure.setInsecure();
    ok = http.begin(secure, url);
  } else {
    ok = http.begin(plain, url);
  }
  if (!ok) return false;
  http.setTimeout(5000);
  http.addHeader("Content-Type", contentType);
  const int code = http.POST(payload);
  http.end();
  return code >= 200 && code < 300;
}

void task(void *) {
  Msg m;
  for (;;) {
    if (xQueueReceive(queue, &m, portMAX_DELAY) != pdTRUE) continue;
    if (WiFi.status() != WL_CONNECTED) continue;  // pas de réseau : message perdu
    if (settings.ntfyTopic.length()) {
      // Publication JSON sur la racine du serveur : gère l'UTF-8 dans le
      // titre (les en-têtes HTTP « Title » sont limités à l'ASCII).
      JsonDocument doc;
      doc["topic"] = settings.ntfyTopic;
      doc["title"] = m.title;
      doc["message"] = m.body;
      doc["priority"] = m.prio;
      if (m.tags[0]) {
        JsonArray tags = doc["tags"].to<JsonArray>();
        String t = m.tags;
        int start = 0;
        while (start < (int)t.length()) {
          int comma = t.indexOf(',', start);
          if (comma < 0) comma = t.length();
          tags.add(t.substring(start, comma));
          start = comma + 1;
        }
      }
      String payload;
      serializeJson(doc, payload);
      String url = settings.ntfyServer;
      if (!url.endsWith("/")) url += "/";
      post(url, "application/json", payload);
    }
    if (settings.webhookUrl.length()) {
      JsonDocument doc;
      doc["title"] = m.title;
      doc["message"] = m.body;
      doc["priority"] = m.prio;
      doc["tags"] = m.tags;
      String payload;
      serializeJson(doc, payload);
      post(settings.webhookUrl, "application/json", payload);
    }
  }
}

}  // namespace

void notifyBegin() {
  queue = xQueueCreate(6, sizeof(Msg));
  xTaskCreatePinnedToCore(task, "notify", 8192, nullptr, 1, nullptr, 0);
}

bool notify(const char *title, const char *body, NotifyPrio prio, const char *tags) {
  if (!queue) return false;
  Msg m{};
  strlcpy(m.title, title, sizeof(m.title));
  strlcpy(m.body, body, sizeof(m.body));
  strlcpy(m.tags, tags, sizeof(m.tags));
  m.prio = (uint8_t)prio;
  return xQueueSend(queue, &m, 0) == pdTRUE;
}
