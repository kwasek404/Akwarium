#ifndef TIME_CONTROLLER_H
#define TIME_CONTROLLER_H

#include <virtuabotixRTC.h>
#include "Debug.h"
#include <TimeLib.h>
#include "Constants.h"
#include "Settings.h"
#include "Timezones.h"

class TimeController {
public:
    TimeController(uint8_t clk, uint8_t dat, uint8_t rst) : rtc(clk, dat, rst) {}

    void begin() {
        // Perform initial time sync from RTC to TimeLib at startup.
        // This ensures that the system time is correct from the very beginning.
        time_t initialTime = nowUTC();
        ::setTime(initialTime);
        DEBUG_PRINTF("Time: Initial sync from RTC. UTC timestamp: %lu\n", initialTime);
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

    void setTime(int year, int month, int day, int hour, int minute, int second, const Settings& settings) {
        tmElements_t tm;
        tm.Year = year - 1970;
        tm.Month = month;
        tm.Day = day;
        tm.Hour = hour;
        tm.Minute = minute;
        tm.Second = second;
        time_t localTime = makeTime(tm);
        
        time_t utcTime = (settings.timezone == TZ_WARSAW) ? warsawTZ.toUTC(localTime) : localTime;

        // Day of week is not used by this RTC module, so we pass 0.
        rtc.setDS1302Time(::second(utcTime), ::minute(utcTime), ::hour(utcTime), 0, ::day(utcTime), ::month(utcTime), ::year(utcTime));
    }

    void adjustTime(long adjustment, const Settings& settings) {
        time_t newTime = nowUTC() + adjustment; // Adjustment is applied to UTC
        DEBUG_PRINTF("Time: Adjusting time by %ld seconds. New UTC: %lu\n", adjustment, newTime);
        rtc.setDS1302Time(::second(newTime), ::minute(newTime), ::hour(newTime), 0, ::day(newTime), ::month(newTime), ::year(newTime));
    }
private:
    virtuabotixRTC rtc;
};

#endif // TIME_CONTROLLER_H