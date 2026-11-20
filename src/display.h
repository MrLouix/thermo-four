// Affichage abstrait : l'OLED SH1106 en v1, un TFT ST7789 pourra
// implémenter la même interface sans toucher au reste du code.
#pragma once
#include <Arduino.h>

struct UiModel {
  String probeName[2];
  float temp[2];
  bool valid[2];
  float target;          // consigne de la sonde affichée (NaN si aucune)
  uint8_t shown;
  int batteryPct;        // -1 = inconnu
  bool wifiOk;
  bool recording;
  uint32_t timerS;       // 0 = pas de minuteur
  bool alarm;
  bool showIp;
  String ip;
  String apInfo;         // non vide en mode configuration
};

class Display {
 public:
  virtual ~Display() = default;
  virtual bool begin() = 0;
  virtual void render(const UiModel &m) = 0;
  virtual void sleep(bool off) = 0;
};

Display &displayInstance();
