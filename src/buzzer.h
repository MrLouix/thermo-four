// Buzzer piézo passif : motifs sonores non bloquants et volume réglable.
#pragma once
#include <cstdint>

enum class Sound : uint8_t {
  Click,      // appui bouton
  Target,     // consigne atteinte (répété jusqu'à acquittement)
  PreAlert,   // pré-alerte
  Low,        // four retombé sous le seuil bas
  Timer,      // fin du minuteur (répété jusqu'à acquittement)
  Fault,      // sonde débranchée
  Battery,    // batterie faible
  Test,       // test depuis l'interface web
};

void buzzerBegin();
void buzzerPlay(Sound s);
void buzzerStop();
bool buzzerBusy();
void buzzerLoop();  // à appeler dans loop()
