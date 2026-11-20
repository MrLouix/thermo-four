// Wi-Fi : réseaux mémorisés (box, partage de connexion) + portail de
// configuration en point d'accès, mDNS et heure NTP.
#pragma once
#include <Arduino.h>

void netBegin(bool forcePortal);
void netLoop();
bool netConnected();
bool netPortalActive();
String netIp();
String netApInfo();     // "SSID / mot de passe" en mode portail, sinon vide
bool netTimeValid();    // heure NTP disponible
