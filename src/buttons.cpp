#include "buttons.h"

#include <Arduino.h>

#include "config.h"

namespace {

constexpr uint32_t DEBOUNCE_MS = 25;

struct Btn {
  uint8_t pin;
  bool stable = false;      // true = appuyé
  bool lastRaw = false;
  uint32_t changedAt = 0;
  uint32_t pressedAt = 0;
  uint32_t lastRepeat = 0;
  bool longSent = false;
  bool powerSent = false;
  bool repeating = false;
};

Btn plus{pins::BTN_PLUS}, minus{pins::BTN_MINUS}, mode{pins::BTN_MODE};

// Retourne +1 à l'appui, -1 au relâchement, 0 sinon.
int8_t debounce(Btn &b, uint32_t now) {
  const bool raw = digitalRead(b.pin) == LOW;
  if (raw != b.lastRaw) {
    b.lastRaw = raw;
    b.changedAt = now;
  }
  if (raw != b.stable && now - b.changedAt >= DEBOUNCE_MS) {
    b.stable = raw;
    if (raw) {
      b.pressedAt = now;
      b.longSent = b.powerSent = b.repeating = false;
      return 1;
    }
    return -1;
  }
  return 0;
}

BtnEvent pollStep(Btn &b, uint32_t now, BtnEvent shortEv, BtnEvent repeatEv) {
  const int8_t e = debounce(b, now);
  if (e == -1 && !b.repeating) return shortEv;
  if (b.stable) {
    const uint32_t held = now - b.pressedAt;
    if (!b.repeating && held >= timing::REPEAT_DELAY_MS) {
      b.repeating = true;
      b.lastRepeat = now;
      return repeatEv;
    }
    if (b.repeating && now - b.lastRepeat >= timing::REPEAT_RATE_MS) {
      b.lastRepeat = now;
      return repeatEv;
    }
  }
  return BtnEvent::None;
}

}  // namespace

void buttonsBegin() {
  for (Btn *b : {&plus, &minus, &mode}) pinMode(b->pin, INPUT_PULLUP);
}

BtnEvent buttonsPoll() {
  const uint32_t now = millis();
  BtnEvent ev = pollStep(plus, now, BtnEvent::PlusShort, BtnEvent::PlusRepeat);
  if (ev != BtnEvent::None) return ev;
  ev = pollStep(minus, now, BtnEvent::MinusShort, BtnEvent::MinusRepeat);
  if (ev != BtnEvent::None) return ev;

  const int8_t e = debounce(mode, now);
  if (e == -1 && !mode.longSent) return BtnEvent::ModeShort;
  if (mode.stable) {
    const uint32_t held = now - mode.pressedAt;
    if (!mode.powerSent && held >= timing::POWER_PRESS_MS) {
      mode.powerSent = true;
      return BtnEvent::ModePower;
    }
    if (!mode.longSent && held >= timing::LONG_PRESS_MS) {
      mode.longSent = true;
      return BtnEvent::ModeLong;
    }
  }
  return BtnEvent::None;
}
