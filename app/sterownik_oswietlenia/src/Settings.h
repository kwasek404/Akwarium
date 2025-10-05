#ifndef SETTINGS_H
#define SETTINGS_H

#include <EEPROM.h>
#include "Constants.h"

enum TimezoneSetting { TZ_UTC, TZ_WARSAW };

struct Settings {
    int startHour = 8;
    int startMinute = 0;
    int stopHour = 20;
    int stopMinute = 0;
    TimezoneSetting timezone = TZ_WARSAW;

    void load() {
        startHour = EEPROM.read(EEPROM_TIMER_START_HOUR_ADDR);
        startMinute = EEPROM.read(EEPROM_TIMER_START_MINUTE_ADDR);
        stopHour = EEPROM.read(EEPROM_TIMER_STOP_HOUR_ADDR);
        stopMinute = EEPROM.read(EEPROM_TIMER_STOP_MINUTE_ADDR);
        timezone = (TimezoneSetting)EEPROM.read(EEPROM_TIMEZONE_ADDR);
        if (timezone > TZ_WARSAW) timezone = TZ_WARSAW; // Basic validation
    }

    void save() {
        EEPROM.write(EEPROM_TIMER_START_HOUR_ADDR, startHour);
        EEPROM.write(EEPROM_TIMER_START_MINUTE_ADDR, startMinute);
        EEPROM.write(EEPROM_TIMER_STOP_HOUR_ADDR, stopHour);
        EEPROM.write(EEPROM_TIMER_STOP_MINUTE_ADDR, stopMinute);
        EEPROM.write(EEPROM_TIMEZONE_ADDR, timezone);
    }
};

#endif // SETTINGS_H