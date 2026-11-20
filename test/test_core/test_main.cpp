#include <unity.h>

#include "alarm.h"
#include "thermistor.h"

using namespace thermo;

void setUp() {}
void tearDown() {}

static double rBeta(double r25, double beta, double tC) {
  return r25 * std::exp(beta * (1.0 / (tC + KELVIN) - 1.0 / (25.0 + KELVIN)));
}

void test_beta_round_trip() {
  const ShCoeffs k = fromBeta(100000, 3950);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 25.0, temperatureC(100000, k));
  TEST_ASSERT_FLOAT_WITHIN(0.01, 200.0, temperatureC(rBeta(100000, 3950, 200), k));
}

void test_three_points_recovers_beta_curve() {
  ShCoeffs k;
  TEST_ASSERT_TRUE(fromThreePoints(0, rBeta(100000, 3950, 0), 25, 100000, 100,
                                   rBeta(100000, 3950, 100), k));
  TEST_ASSERT_FLOAT_WITHIN(0.05, 0.0, temperatureC(rBeta(100000, 3950, 0), k));
  TEST_ASSERT_FLOAT_WITHIN(0.05, 100.0, temperatureC(rBeta(100000, 3950, 100), k));
}

void test_three_points_rejects_degenerate() {
  ShCoeffs k;
  TEST_ASSERT_FALSE(fromThreePoints(0, 1000, 25, 1000, 100, 500, k));
}

void test_ratio_and_status() {
  TEST_ASSERT_TRUE(statusFromRatio(0.0) == ProbeStatus::Short);
  TEST_ASSERT_TRUE(statusFromRatio(0.999) == ProbeStatus::Open);
  TEST_ASSERT_TRUE(statusFromRatio(0.5) == ProbeStatus::Ok);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 3300.0, resistanceFromRatio(0.5, 3300));
}

void test_high_alarm_fires_once_with_hysteresis() {
  AlarmConfig cfg;
  cfg.highOn = true;
  cfg.high = 60;
  AlarmChannel ch;
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 59.0f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_HIGH, ch.update(cfg, true, 60.0f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 59.5f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 60.2f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 58.5f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_HIGH, ch.update(cfg, true, 61.0f, false));
}

void test_pre_alert() {
  AlarmConfig cfg;
  cfg.highOn = cfg.preOn = true;
  cfg.high = 60;
  cfg.preDelta = 5;
  AlarmChannel ch;
  TEST_ASSERT_EQUAL_UINT8(EV_PRE, ch.update(cfg, true, 55.5f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 57.0f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_HIGH, ch.update(cfg, true, 60.0f, false));
}

void test_low_alarm_arms_after_rising() {
  AlarmConfig cfg;
  cfg.lowOn = true;
  cfg.low = 150;
  AlarmChannel ch;
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 100.0f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 180.0f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_LOW, ch.update(cfg, true, 149.0f, false));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, true, 148.0f, false));
}

void test_fault_only_when_watched() {
  AlarmConfig cfg;
  AlarmChannel ch;
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, false, 0, false));
  TEST_ASSERT_EQUAL_UINT8(EV_FAULT, ch.update(cfg, false, 0, true));
  TEST_ASSERT_EQUAL_UINT8(EV_NONE, ch.update(cfg, false, 0, true));
  TEST_ASSERT_EQUAL_UINT8(EV_FAULT_CLEARED, ch.update(cfg, true, 20, true));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_beta_round_trip);
  RUN_TEST(test_three_points_recovers_beta_curve);
  RUN_TEST(test_three_points_rejects_degenerate);
  RUN_TEST(test_ratio_and_status);
  RUN_TEST(test_high_alarm_fires_once_with_hysteresis);
  RUN_TEST(test_pre_alert);
  RUN_TEST(test_low_alarm_arms_after_rising);
  RUN_TEST(test_fault_only_when_watched);
  return UNITY_END();
}
