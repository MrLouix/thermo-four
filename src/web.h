// Serveur web embarqué : interface (LittleFS /www), API REST, flux SSE.
#pragma once

void webBegin();
void webLoop();  // diffusion SSE périodique
// Demande une sauvegarde différée des réglages (après une rafale de clics).
void requestSettingsSave();
