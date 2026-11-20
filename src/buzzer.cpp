#include "buzzer.h"

#include <Arduino.h>

#include "config.h"
#include "settings.h"

// Volume = rapport cyclique du PWM (résolution 10 bits). 50 % = maximum
// pour un piézo. Réglage grossier : à ajuster sur le prototype.
static const uint16_t DUTY_BY_VOLUME[4] = {0, 20, 90, 512};
static const uint8_t LEDC_CH = 0;

struct Step {
  uint16_t freq;  // 0 = silence
  uint16_t ms;
};

struct Pattern {
  const Step *steps;
  uint8_t count;
  bool repeat;
};

static const Step S_CLICK[] = {{hw::BUZZER_FREQ, 15}};
static const Step S_TARGET[] = {{4000, 150}, {0, 80}, {4000, 150}, {0, 80}, {4000, 150}, {0, 900}};
static const Step S_PRE[] = {{3200, 120}, {0, 120}, {3200, 120}};
static const Step S_LOW[] = {{2500, 400}, {0, 200}, {2000, 400}};
static const Step S_TIMER[] = {{3500, 100}, {0, 100}, {3500, 100}, {0, 100}, {3500, 100}, {0, 100}, {3500, 100}, {0, 800}};
static const Step S_FAULT[] = {{1800, 600}, {0, 200}, {1800, 600}};
static const Step S_BATT[] = {{2200, 80}, {0, 80}, {1800, 200}};
static const Step S_TEST[] = {{hw::BUZZER_FREQ, 300}};

#define PAT(a, r) Pattern{a, (uint8_t)(sizeof(a) / sizeof(a[0])), r}

static Pattern patternFor(Sound s) {
  switch (s) {
    case Sound::Click: return PAT(S_CLICK, false);
    case Sound::Target: return PAT(S_TARGET, true);
    case Sound::PreAlert: return PAT(S_PRE, false);
    case Sound::Low: return PAT(S_LOW, false);
    case Sound::Timer: return PAT(S_TIMER, true);
    case Sound::Fault: return PAT(S_FAULT, false);
    case Sound::Battery: return PAT(S_BATT, false);
    case Sound::Test: return PAT(S_TEST, false);
  }
  return PAT(S_CLICK, false);
}

static Pattern current{nullptr, 0, false};
static uint8_t idx = 0;
static uint32_t stepEnd = 0;
static bool playing = false;

static void output(uint16_t freq) {
  const uint16_t duty = DUTY_BY_VOLUME[settings.volume & 3];
  if (freq == 0 || duty == 0) {
    ledcWrite(LEDC_CH, 0);
    return;
  }
  ledcWriteTone(LEDC_CH, freq);  // remet le rapport cyclique à 50 %
  ledcWrite(LEDC_CH, duty);
}

static void startStep() {
  const Step &s = current.steps[idx];
  output(s.freq);
  stepEnd = millis() + s.ms;
}

void buzzerBegin() {
  ledcSetup(LEDC_CH, hw::BUZZER_FREQ, 10);
  ledcAttachPin(pins::BUZZER, LEDC_CH);
  ledcWrite(LEDC_CH, 0);
}

void buzzerPlay(Sound s) {
  // Un clic n'interrompt pas une alarme en cours.
  if (s == Sound::Click && playing && current.repeat) return;
  current = patternFor(s);
  idx = 0;
  playing = true;
  startStep();
}

void buzzerStop() {
  playing = false;
  ledcWrite(LEDC_CH, 0);
}

bool buzzerBusy() { return playing; }

void buzzerLoop() {
  if (!playing || (int32_t)(millis() - stepEnd) < 0) return;
  idx++;
  if (idx >= current.count) {
    if (!current.repeat) {
      buzzerStop();
      return;
    }
    idx = 0;
  }
  startStep();
}
