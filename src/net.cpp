#include "net.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <time.h>

#include "config.h"
#include "settings.h"

static WiFiMulti multi;
static bool portal = false;
static bool mdnsStarted = false;
static String apSsid;
static uint32_t lastTry = 0;

static void startPortal() {
  portal = true;
  char suffix[8];
  snprintf(suffix, sizeof(suffix), "%04X", (unsigned)(ESP.getEfuseMac() & 0xFFFF));
  apSsid = String(AP_SSID_PREFIX) + suffix;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str(), settings.apPass.c_str());
}

void netBegin(bool forcePortal) {
  WiFi.setHostname(HOSTNAME);
  if (forcePortal || !settings.hasWifi()) {
    startPortal();
    return;
  }
  WiFi.mode(WIFI_STA);
  for (const auto &w : settings.wifi)
    if (w.ssid.length()) multi.addAP(w.ssid.c_str(), w.pass.c_str());
  configTzTime(TZ_PARIS, "pool.ntp.org", "time.google.com");
  multi.run(10000);
}

void netLoop() {
  if (portal) return;
  const uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    mdnsStarted = false;
    if (now - lastTry > 10000) {  // nouvel essai, sans bloquer longtemps
      lastTry = now;
      multi.run(2000);
    }
    return;
  }
  if (!mdnsStarted) {
    mdnsStarted = MDNS.begin(HOSTNAME);
    if (mdnsStarted) MDNS.addService("http", "tcp", 80);
  }
}

bool netConnected() { return !portal && WiFi.status() == WL_CONNECTED; }
bool netPortalActive() { return portal; }

String netIp() {
  if (portal) return WiFi.softAPIP().toString();
  return netConnected() ? WiFi.localIP().toString() : String("non connecté");
}

String netApInfo() {
  if (!portal) return String();
  return apSsid + " / " + settings.apPass;
}

bool netTimeValid() { return time(nullptr) > 1700000000; }  // après nov. 2023
