// Batterie (jauge MAX17048 de la Feather) et extinction.
#pragma once

void powerBegin();
void powerUpdate();          // met à jour app.batteryPct / app.batteryV
[[noreturn]] void powerOff(); // veille profonde, réveil par le bouton Mode
