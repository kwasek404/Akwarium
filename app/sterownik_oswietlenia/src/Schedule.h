#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

const uint8_t BALLAST_1 = 1 << 0; // 2 tubes: Grolux + Aquastar
const uint8_t BALLAST_2 = 1 << 1; // 2 tubes: Grolux + Aquastar
const uint8_t BALLAST_3 = 1 << 2; // 1 tube:  Grolux

enum class PhaseType {
    RAMP_LINEAR,
    RAMP_QUAD_IN,
    RAMP_QUAD_OUT,
    HOLD
};

struct SchedulePhase {
    const char* name;
    float startPercent;
    float endPercent;
    PhaseType type;
    float startPower;
    float endPower;
};

const float SIESTA_START_PERCENT_OF_DAY = 0.30f;
const float SIESTA_END_PERCENT_OF_DAY = 0.65f;

// Morning: B3 solo -> B3+primary -> hold -> ramp down
// Power thresholds: 0-20% = B3 solo (1 tube), 20-60% = B3+pair (3 tubes)
const int PRO_SCHEDULE_MORNING_PHASES_COUNT = 4;
const SchedulePhase PRO_SCHEDULE_MORNING[PRO_SCHEDULE_MORNING_PHASES_COUNT] = {
    { "Dawn",      0.00,  0.25,   PhaseType::RAMP_QUAD_IN,    6,  10},
    { "Sunrise",   0.25,  0.50,   PhaseType::RAMP_LINEAR,    10,  60},
    { "Morning",   0.50,  0.90,   PhaseType::HOLD,           60,  60},
    { "SiestaR",   0.90,  1.00,   PhaseType::RAMP_QUAD_OUT,  60,   0}
};

// Evening: B3 solo -> B3+primary -> B3+primary+secondary -> hold -> ramp down
// Power thresholds: 0-20% = B3 solo (1 tube), 20-40% = B3+primary (3 tubes), 40-100% = all 5
const int PRO_SCHEDULE_EVENING_PHASES_COUNT = 5;
const SchedulePhase PRO_SCHEDULE_EVENING[PRO_SCHEDULE_EVENING_PHASES_COUNT] = {
    { "Awakening", 0.00,  0.15,   PhaseType::RAMP_QUAD_IN,    6,  40},
    { "ZenithRmp", 0.15,  0.25,   PhaseType::RAMP_LINEAR,    40, 100},
    { "Zenith",    0.25,  0.80,   PhaseType::HOLD,          100, 100},
    { "ZenithD",   0.80,  0.85,   PhaseType::RAMP_LINEAR,   100,  40},
    { "Dusk",      0.85,  1.00,   PhaseType::RAMP_QUAD_OUT,  40,   0}
};

#endif // SCHEDULE_H
