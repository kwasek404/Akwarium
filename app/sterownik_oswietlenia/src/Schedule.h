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
    float startPower; // Represents Total System Power % (0-100)
    float endPower;   // Represents Total System Power % (0-100)
};

// The schedule is defined across two "active light" blocks separated by a siesta.
const float SIESTA_START_PERCENT_OF_DAY = 0.37f;
const float SIESTA_END_PERCENT_OF_DAY = 0.67f;

const int PRO_SCHEDULE_MORNING_PHASES_COUNT = 4;
const SchedulePhase PRO_SCHEDULE_MORNING[PRO_SCHEDULE_MORNING_PHASES_COUNT] = {
    // Name,        Start%, End%,  Ballasts,                  Type,                  StartP, EndP
    { "Dawn",      0.00,  0.25,   BALLAST_3,                 PhaseType::RAMP_QUAD_IN,    0,  10}, // Ramp to 1 tube @ 50% = 10% system power
    { "Sunrise",   0.25,  0.50,   BALLAST_3 | BALLAST_1,     PhaseType::RAMP_LINEAR,    10,  60}, // Ramp to 3 tubes @ 100% = 60% system power
    { "Morning",   0.50,  0.90,   BALLAST_3 | BALLAST_1,     PhaseType::HOLD,          60,  60}, // Hold 3 tubes @ 100%
    { "SiestaRamp",0.90,  1.00,   BALLAST_3 | BALLAST_1,     PhaseType::RAMP_QUAD_OUT, 60,   0}  // Ramp down
};

const int PRO_SCHEDULE_EVENING_PHASES_COUNT = 5;
const SchedulePhase PRO_SCHEDULE_EVENING[PRO_SCHEDULE_EVENING_PHASES_COUNT] = {
    // Name,        Start%, End%,  Ballasts,                      Type,                  StartP, EndP
    { "Awakening", 0.00,  0.15,   BALLAST_1 | BALLAST_2,         PhaseType::RAMP_QUAD_IN,    0,  64}, // Ramp to 4 tubes @ 80% = 64% system power
    { "ZenithRamp",0.15,  0.25,   BALLAST_1|BALLAST_2|BALLAST_3, PhaseType::RAMP_LINEAR,    64, 100}, // Ramp to 5 tubes @ 100% = 100% system power
    { "Zenith",    0.25,  0.80,   BALLAST_1|BALLAST_2|BALLAST_3, PhaseType::HOLD,         100, 100}, // Hold 5 tubes @ 100%
    { "ZenithDrop",0.80,  0.85,   BALLAST_1|BALLAST_2|BALLAST_3, PhaseType::RAMP_LINEAR,   100,  90}, // Drop power before switching off ballasts
    { "Dusk",      0.85,  1.00,   BALLAST_1,                     PhaseType::RAMP_QUAD_OUT,  90,   0}  // Ramp down with 2 tubes
};

#endif // SCHEDULE_H