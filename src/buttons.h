// Lecture des 3 boutons : anti-rebond, appui court/long, répétition.
#pragma once
#include <cstdint>

enum class BtnEvent : uint8_t {
  None,
  PlusShort,
  PlusRepeat,   // émis en continu pendant un appui long
  MinusShort,
  MinusRepeat,
  ModeShort,    // changer de sonde / couper l'alarme
  ModeLong,     // 1,5 s : afficher l'IP
  ModePower,    // 4 s : extinction
};

void buttonsBegin();
BtnEvent buttonsPoll();  // un événement par appel au plus
