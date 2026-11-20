// Logique applicative commune aux boutons et à l'interface web.
#pragma once
#include <ArduinoJson.h>

void controllerBegin();
void controllerTick();                       // mesure + alarmes (1 Hz)
void controllerLoop();                       // minuteur, batterie…
void acknowledgeAlarm();                     // couper le buzzer
void adjustTarget(uint8_t probe, float delta);
void timerStart(uint32_t seconds);
void timerStop();
void stateJson(JsonObject o);

// Calibration : enregistre la résistance actuelle comme point n (0..2).
const char *calibCapture(uint8_t probe, uint8_t point, float tempC);
const char *calibApply(uint8_t probe);
const char *calibFromBeta(uint8_t probe, double r25, double beta);
void calibJson(uint8_t probe, JsonObject o);
