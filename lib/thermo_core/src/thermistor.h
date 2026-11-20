// Calculs thermistance NTC : pont diviseur et Steinhart-Hart.
// Code pur C++, sans dépendance Arduino, testé en natif (test/test_core).
#pragma once
#include <cmath>

namespace thermo {

constexpr double KELVIN = 273.15;

// Coefficients de Steinhart-Hart : 1/T = a + b·ln(R) + c·ln(R)^3 (T en kelvins)
struct ShCoeffs {
  double a;
  double b;
  double c;
};

enum class ProbeStatus { Ok, Open, Short };

// Seuils du rapport V(sonde)/V(excitation) en dessous/au-dessus desquels on
// considère la sonde en court-circuit ou absente.
constexpr double RATIO_SHORT = 0.002;
constexpr double RATIO_OPEN = 0.995;

// Pont : EXC — R_ref — point milieu — NTC — GND.
// ratio = V(point milieu) / V(excitation) = R_ntc / (R_ref + R_ntc)
inline ProbeStatus statusFromRatio(double ratio) {
  if (!(ratio > RATIO_SHORT)) return ProbeStatus::Short;  // couvre aussi NaN
  if (ratio >= RATIO_OPEN) return ProbeStatus::Open;
  return ProbeStatus::Ok;
}

inline double resistanceFromRatio(double ratio, double rRef) {
  return rRef * ratio / (1.0 - ratio);
}

inline double temperatureC(double r, const ShCoeffs &k) {
  const double l = std::log(r);
  return 1.0 / (k.a + k.b * l + k.c * l * l * l) - KELVIN;
}

// Modèle « bêta » (R25 + B) exprimé en coefficients Steinhart-Hart (c = 0).
inline ShCoeffs fromBeta(double r25, double beta) {
  const double t0 = 25.0 + KELVIN;
  return {1.0 / t0 - std::log(r25) / beta, 1.0 / beta, 0.0};
}

// Calibration sur 3 points (températures en °C, résistances en ohms).
// Retourne false si les points sont dégénérés (températures ou résistances
// non distinctes, ou résultat non physique).
inline bool fromThreePoints(double t1, double r1, double t2, double r2,
                            double t3, double r3, ShCoeffs &out) {
  if (r1 <= 0 || r2 <= 0 || r3 <= 0) return false;
  const double l1 = std::log(r1), l2 = std::log(r2), l3 = std::log(r3);
  const double y1 = 1.0 / (t1 + KELVIN), y2 = 1.0 / (t2 + KELVIN),
               y3 = 1.0 / (t3 + KELVIN);
  if (l2 == l1 || l3 == l1 || l3 == l2) return false;
  const double g2 = (y2 - y1) / (l2 - l1);
  const double g3 = (y3 - y1) / (l3 - l1);
  const double c = (g3 - g2) / (l3 - l2) / (l1 + l2 + l3);
  const double b = g2 - c * (l1 * l1 + l1 * l2 + l2 * l2);
  const double a = y1 - (b + l1 * l1 * c) * l1;
  if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || b <= 0)
    return false;
  out = {a, b, c};
  return true;
}

}  // namespace thermo
