// Mesure des sondes NTC via l'ADS1115 (mesure ratiométrique).
#pragma once
#include "state.h"

bool probesBegin();
// Effectue une mesure de toutes les sondes et met à jour app.probe[].
void probesMeasure();
// Résistance brute de la sonde i, pour la calibration (NaN si invalide).
double probeResistance(uint8_t i);
