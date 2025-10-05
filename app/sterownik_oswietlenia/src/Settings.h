#ifndef SETTINGS_H
#define SETTINGS_H

#include <EEPROM.h>
#include "Debug.h"
#include "Constants.h"

enum TimezoneSetting { TZ_UTC, TZ_WARSAW };

struct Settings {
    int startHour = 8;
    int startMinute = 0;
    int stopHour = 20;
    int stopMinute = 0;
    TimezoneSetting timezone = TZ_WARSAW;

    void load() {
        DEBUG_PRINTLN("Settings: Loading from EEPROM...");
        startHour = EEPROM.read(EEPROM_TIMER_START_HOUR_ADDR);
        startMinute = EEPROM.read(EEPROM_TIMER_START_MINUTE_ADDR);
        stopHour = EEPROM.read(EEPROM_TIMER_STOP_HOUR_ADDR);
        stopMinute = EEPROM.read(EEPROM_TIMER_STOP_MINUTE_ADDR);
        timezone = (TimezoneSetting)EEPROM.read(EEPROM_TIMEZONE_ADDR);
        if (timezone > TZ_WARSAW) timezone = TZ_WARSAW; // Basic validation
        DEBUG_PRINTF("Settings: Start=%02d:%02d, Stop=%02d:%02d, TZ=%d\n", startHour, startMinute, stopHour, stopMinute, timezone);
    }

    void save() {
        DEBUG_PRINTLN("Settings: Saving to EEPROM...");
        EEPROM.write(EEPROM_TIMER_START_HOUR_ADDR, startHour);
        EEPROM.write(EEPROM_TIMER_START_MINUTE_ADDR, startMinute);
        EEPROM.write(EEPROM_TIMER_STOP_HOUR_ADDR, stopHour);
        EEPROM.write(EEPROM_TIMER_STOP_MINUTE_ADDR, stopMinute);
        EEPROM.write(EEPROM_TIMEZONE_ADDR, timezone);
        DEBUG_PRINTLN("Settings: Save complete.");
    }
};

#endif // SETTINGS_H