// Brochage et constantes matérielles. Voir docs/cablage.md.
#pragma once
#include <cstdint>

namespace pins {
constexpr uint8_t SDA = 3;
constexpr uint8_t SCL = 4;
constexpr uint8_t I2C_PWR = 7;    // alimentation du connecteur STEMMA QT
constexpr uint8_t BTN_PLUS = 5;   // D5
constexpr uint8_t BTN_MINUS = 6;  // D6
constexpr uint8_t BTN_MODE = 9;   // D9 — réveil de la veille profonde
constexpr uint8_t BUZZER = 10;    // D10
constexpr uint8_t PROBE_EXC = 11; // D11 — excitation des ponts
constexpr uint8_t LED = 13;       // D13
}  // namespace pins

namespace hw {
constexpr uint8_t ADS_ADDR = 0x48;
constexpr uint8_t ADS_CH_PROBE[2] = {0, 1};
constexpr uint8_t ADS_CH_EXC = 2;
constexpr uint8_t PROBE_COUNT = 2;

// Valeurs par défaut tant que les sondes ne sont pas calibrées.
constexpr double DEFAULT_RREF = 3300.0;   // ohms, voir docs/cablage.md
constexpr double DEFAULT_R25 = 100000.0;  // ohms
constexpr double DEFAULT_BETA = 3950.0;

constexpr uint32_t BUZZER_FREQ = 4000;    // Hz, à ajuster sur la résonance du piézo
}  // namespace hw

namespace timing {
constexpr uint32_t MEASURE_MS = 1000;
constexpr uint32_t SSE_MS = 1000;
constexpr uint32_t LONG_PRESS_MS = 1500;      // Mode : afficher l'IP
constexpr uint32_t POWER_PRESS_MS = 4000;     // Mode : extinction
constexpr uint32_t REPEAT_DELAY_MS = 600;     // +/- : début du défilement rapide
constexpr uint32_t REPEAT_RATE_MS = 120;
}  // namespace timing

constexpr const char *HOSTNAME = "thermo";              // http://thermo.local
constexpr const char *AP_SSID_PREFIX = "Thermo-Four-";
constexpr const char *AP_DEFAULT_PASS = "thermofour";   // modifiable dans les réglages
constexpr const char *TZ_PARIS = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr uint8_t BATTERY_LOW_PCT = 15;
constexpr uint8_t MAX_WIFI = 3;
constexpr uint8_t MAX_LOG_FILES = 10;
