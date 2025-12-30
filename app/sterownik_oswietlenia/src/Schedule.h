#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

// Define bitmasks for the ballasts for clarity
// Ballast 1: Grolux + Aquastar Pair 1 (used for Dusk)
// Ballast 2: Grolux + Aquastar Pair 2
// Ballast 3: Grolux Single (used for Dawn)
const uint8_t BALLAST_1 = 1 << 0; // = 1
const uint8_t BALLAST_2 = 1 << 1; // = 2
const uint8_t BALLAST_3 = 1 << 2; // = 4

enum class PhaseType {
    RAMP_LINEAR,   // Linear y = x
    RAMP_QUAD_IN,  // Quadratic ease-in y = x^2
    RAMP_QUAD_OUT, // Quadratic ease-out y = 1 - (1-x)^2
    HOLD           // Hold power constant
};

struct SchedulePhase {
    const char* name;
    float startPercent;
    float endPercent;
    uint8_t ballastMask;
    PhaseType type;
    float startPower;
    float endPower;
};

const float SIESTA_START_PERCENT_OF_DAY = 0.37f;
const float SIESTA_END_PERCENT_OF_DAY = 0.67f;

const int PRO_SCHEDULE_MORNING_PHASES_COUNT = 4;
const SchedulePhase PRO_SCHEDULE_MORNING[PRO_SCHEDULE_MORNING_PHASES_COUNT] = {
    // Name,        Start%, End%,  Ballasts,                  Type,                 StartP, EndP
    { "Dawn",      0.00,  0.25,   BALLAST_3,                 PhaseType::RAMP_QUAD_IN,   0,  50},
    { "Sunrise",   0.25,  0.50,   BALLAST_3 | BALLAST_1,     PhaseType::RAMP_LINEAR, 50, 100},
    { "Morning",   0.50,  0.90,   BALLAST_3 | BALLAST_1,     PhaseType::HOLD,       100, 100},
    { "SiestaRamp",0.90,  1.00,   BALLAST_3 | BALLAST_1,     PhaseType::RAMP_QUAD_OUT,100,   0}
};

const int PRO_SCHEDULE_EVENING_PHASES_COUNT = 4;
const SchedulePhase PRO_SCHEDULE_EVENING[PRO_SCHEDULE_EVENING_PHASES_COUNT] = {
    // Name,        Start%, End%,  Ballasts,                      Type,                  StartP, EndP
    { "Awakening", 0.00,  0.15,   BALLAST_1 | BALLAST_2,         PhaseType::RAMP_QUAD_IN,    0,  80},
    { "ZenithRamp",0.15,  0.25,   BALLAST_1|BALLAST_2|BALLAST_3, PhaseType::RAMP_LINEAR,  80, 100},
    { "Zenith",    0.25,  0.85,   BALLAST_1|BALLAST_2|BALLAST_3, PhaseType::HOLD,        100, 100},
    { "Dusk",      0.85,  1.00,   BALLAST_1,                     PhaseType::RAMP_QUAD_OUT, 100,   0}
};

#endif // SCHEDULE_H
