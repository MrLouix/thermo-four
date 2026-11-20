// Enregistrements programmés : CSV dans LittleFS (/logs).
#pragma once
#include <ArduinoJson.h>
#include <time.h>

// startAt = 0 : démarrage immédiat. Sinon heure Unix (nécessite NTP).
// Retourne un message d'erreur, ou nullptr si OK.
const char *loggerSchedule(time_t startAt, uint32_t durationS, uint16_t intervalS);
void loggerStop();
bool loggerActive();      // enregistrement en cours
bool loggerPending();     // programmé, pas encore démarré
void loggerLoop();
void loggerStatus(JsonObject o);
void loggerList(JsonArray a);
bool loggerDelete(const String &name);
