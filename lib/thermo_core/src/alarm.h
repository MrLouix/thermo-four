// Logique d'alarme d'une sonde : seuil haut, seuil bas, pré-alerte, défaut.
// Une seule notification par franchissement, réarmement avec hystérésis.
#pragma once
#include <cstdint>

namespace thermo {

struct AlarmConfig {
  bool highOn = false;
  float high = 60.0f;      // °C — consigne (viande cuite, four trop chaud)
  bool lowOn = false;
  float low = 150.0f;      // °C — four qui retombe
  bool preOn = false;
  float preDelta = 5.0f;   // °C avant la consigne haute
};

enum AlarmEvent : uint8_t {
  EV_NONE = 0,
  EV_HIGH = 1 << 0,
  EV_LOW = 1 << 1,
  EV_PRE = 1 << 2,
  EV_FAULT = 1 << 3,
  EV_FAULT_CLEARED = 1 << 4,
};

constexpr float HYSTERESIS = 1.0f;

class AlarmChannel {
 public:
  // valid = false si la sonde est absente ou en court-circuit.
  // faultWatch = true si un défaut doit être signalé (enregistrement en cours
  // ou alarme armée).
  uint8_t update(const AlarmConfig &cfg, bool valid, float t, bool faultWatch) {
    uint8_t ev = EV_NONE;
    if (!valid) {
      if (!faultLatched_ && faultWatch) ev |= EV_FAULT;
      if (faultWatch) faultLatched_ = true;
      return ev;
    }
    if (faultLatched_) {
      faultLatched_ = false;
      ev |= EV_FAULT_CLEARED;
    }

    if (cfg.highOn) {
      if (!highLatched_ && t >= cfg.high) {
        highLatched_ = true;
        ev |= EV_HIGH;
      } else if (highLatched_ && t < cfg.high - HYSTERESIS) {
        highLatched_ = false;
      }
      if (cfg.preOn) {
        const float pre = cfg.high - cfg.preDelta;
        if (!preLatched_ && t >= pre && t < cfg.high) {
          preLatched_ = true;
          ev |= EV_PRE;
        } else if (preLatched_ && t < pre - HYSTERESIS) {
          preLatched_ = false;
        }
        // Si la consigne est atteinte directement, la pré-alerte est
        // considérée comme passée.
        if (highLatched_) preLatched_ = true;
      }
    } else {
      highLatched_ = preLatched_ = false;
    }

    if (cfg.lowOn) {
      // Le seuil bas ne s'arme qu'une fois la température passée au-dessus
      // (sinon il sonnerait pendant toute la montée en température du four).
      if (!lowArmed_ && t >= cfg.low + HYSTERESIS) lowArmed_ = true;
      if (lowArmed_ && !lowLatched_ && t <= cfg.low) {
        lowLatched_ = true;
        ev |= EV_LOW;
      } else if (lowLatched_ && t >= cfg.low + HYSTERESIS) {
        lowLatched_ = false;
      }
    } else {
      lowArmed_ = lowLatched_ = false;
    }
    return ev;
  }

  bool highActive() const { return highLatched_; }
  bool lowActive() const { return lowLatched_; }
  bool faultActive() const { return faultLatched_; }
  void reset() { *this = AlarmChannel(); }

 private:
  bool highLatched_ = false;
  bool preLatched_ = false;
  bool lowArmed_ = false;
  bool lowLatched_ = false;
  bool faultLatched_ = false;
};

}  // namespace thermo
