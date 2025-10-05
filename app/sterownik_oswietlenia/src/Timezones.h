#ifndef TIMEZONES_H
#define TIMEZONES_H

#include <Timezone.h>

// Europe/Warsaw Time Zone Rules
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};    // Central European Standard Time
Timezone warsawTZ(CEST, CET);

#endif // TIMEZONES_H