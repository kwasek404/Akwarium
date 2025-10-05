#ifndef TIME_CONTROLLER_H
#define TIME_CONTROLLER_H

#include <virtuabotixRTC.h>
#include <Timezone.h>
#include <TimeLib.h>
#include "Constants.h"
#include "Settings.h"

// Europe/Warsaw Time Zone Rules
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};    // Central European Standard Time
Timezone warsawTZ(CEST, CET);

class TimeController {
public:
    TimeController(uint8_t clk, uint8_t dat, uint8_t rst) : rtc(clk, dat, rst) {}

    void begin() {
        // Set TimeLib to sync with our RTC reading function.
        // This needs to be a static function or a lambda without captures.
        // For simplicity, we will manually sync in the main loop if needed,
        // or rely on the fact that we get the time from the RTC each loop.
        // The setSyncProvider call is removed as it's problematic with classes.
    }

    time_t nowUTC() {
        rtc.updateTime();
        tmElements_t tm;
        tm.Year = rtc.year - 1970;
        tm.Month = rtc.month;
        tm.Day = rtc.dayofmonth;
        tm.Hour = rtc.hours;
        tm.Minute = rtc.minutes;
        tm.Second = rtc.seconds;
        return makeTime(tm);
    }

    time_t toLocal(time_t utc, const Settings& settings) {
        if (settings.timezone == TZ_WARSAW) {
            return warsawTZ.toLocal(utc);
        }
        return utc;
    }

    void setTime(int year, int month, int day, int hour, int minute, int second) {
        // Day of week is not used by this RTC module, so we pass 0
        rtc.setDS1302Time(second, minute, hour, 0, day, month, year);
    }

    void adjustTime(long seconds) {
        // When adjusting, we adjust the UTC time stored in the RTC
        time_t newTime = nowUTC() + seconds;
        setTime(year(newTime), month(newTime), day(newTime), hour(newTime), minute(newTime), second(newTime));
    }
private:
    virtuabotixRTC rtc;
};

#endif // TIME_CONTROLLER_H