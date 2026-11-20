// Thermomètre de four connecté — point d'entrée.
#include <Arduino.h>
#include <LittleFS.h>
#include <Wire.h>

#include "buttons.h"
#include "buzzer.h"
#include "config.h"
#include "controller.h"
#include "display.h"
#include "logger.h"
#include "net.h"
#include "notify.h"
#include "power.h"
#include "probes.h"
#include "settings.h"
#include "state.h"
#include "web.h"

namespace {

uint32_t lastMeasure = 0;
uint32_t lastActivity = 0;
uint32_t lastRender = 0;
bool screenOff = false;
bool savePending = false;
uint32_t saveAt = 0;

void wakeScreen() {
  lastActivity = millis();
  if (screenOff) {
    screenOff = false;
    displayInstance().sleep(false);
  }
}

void handleButton(BtnEvent ev) {
  if (ev == BtnEvent::None) return;
  // Écran éteint : le premier appui ne fait que le rallumer.
  if (screenOff) {
    wakeScreen();
    return;
  }
  wakeScreen();
  if (ev != BtnEvent::PlusRepeat && ev != BtnEvent::MinusRepeat) buzzerPlay(Sound::Click);
  const uint8_t s = app.shownProbe;
  switch (ev) {
    case BtnEvent::PlusShort: adjustTarget(s, 1); requestSettingsSave(); break;
    case BtnEvent::PlusRepeat: adjustTarget(s, 5); requestSettingsSave(); break;
    case BtnEvent::MinusShort: adjustTarget(s, -1); requestSettingsSave(); break;
    case BtnEvent::MinusRepeat: adjustTarget(s, -5); requestSettingsSave(); break;
    case BtnEvent::ModeShort:
      if (app.alarmRinging) acknowledgeAlarm();
      else app.shownProbe = (s + 1) % hw::PROBE_COUNT;
      break;
    case BtnEvent::ModeLong:
      app.showIp = true;
      app.showIpUntil = millis() + 10000;
      break;
    case BtnEvent::ModePower:
      settingsSave();
      loggerStop();
      powerOff();
    default: break;
  }
}

void render() {
  UiModel m;
  for (uint8_t i = 0; i < hw::PROBE_COUNT; i++) {
    m.probeName[i] = settings.probe[i].name;
    m.temp[i] = app.probe[i].tempC;
    m.valid[i] = app.probe[i].valid();
  }
  m.shown = app.shownProbe;
  const auto &a = settings.probe[m.shown].alarm;
  m.target = a.highOn ? a.high : NAN;
  m.batteryPct = app.batteryPct;
  m.wifiOk = netConnected();
  m.recording = loggerActive();
  m.timerS = app.timer.remainingS();
  m.alarm = app.alarmRinging;
  m.showIp = app.showIp;
  m.ip = netIp();
  m.apInfo = netApInfo();
  displayInstance().render(m);
}

}  // namespace

void requestSettingsSave() {
  savePending = true;
  saveAt = millis() + 3000;
}

void setup() {
  Serial.begin(115200);
  pinMode(pins::LED, OUTPUT);
  pinMode(pins::I2C_PWR, OUTPUT);
  digitalWrite(pins::I2C_PWR, HIGH);
  delay(10);
  Wire.begin(pins::SDA, pins::SCL);

  if (!LittleFS.begin(true)) Serial.println("LittleFS indisponible");
  settingsLoad();

  buttonsBegin();
  // Bouton Mode maintenu au démarrage : portail de configuration forcé.
  const bool forcePortal = digitalRead(pins::BTN_MODE) == LOW;

  buzzerBegin();
  if (!displayInstance().begin()) Serial.println("Écran absent");
  if (!probesBegin()) Serial.println("ADS1115 absent");
  powerBegin();
  controllerBegin();
  notifyBegin();
  netBegin(forcePortal);
  webBegin();
  lastActivity = millis();
}

void loop() {
  const uint32_t now = millis();

  handleButton(buttonsPoll());
  buzzerLoop();
  netLoop();

  if (now - lastMeasure >= timing::MEASURE_MS) {
    lastMeasure = now;
    controllerTick();
  }
  controllerLoop();
  loggerLoop();
  webLoop();

  if (app.alarmRinging) wakeScreen();
  if (!screenOff && settings.screenTimeoutS &&
      now - lastActivity > settings.screenTimeoutS * 1000UL && !netPortalActive()) {
    screenOff = true;
    displayInstance().sleep(true);
  }
  if (!screenOff && now - lastRender >= 200) {
    lastRender = now;
    render();
  }

  digitalWrite(pins::LED, loggerActive() && (now / 500) % 2);

  if (savePending && (int32_t)(now - saveAt) > 0) {
    savePending = false;
    settingsSave();
  }
  delay(5);
}
