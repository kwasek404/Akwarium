#ifndef TIME_CONTROLLER_H
#define TIME_CONTROLLER_H

#include <virtuabotixRTC.h>
#include <TimeLib.h>
#include "Constants.h"
#include "Settings.h"
#include "Timezones.h"

class TimeController {
public:
    TimeController(uint8_t clk, uint8_t dat, uint8_t rst) : rtc(clk, dat, rst) {}

    void begin() {
        time_t initialTime = getRawRtcTime();
        ::setTime(initialTime);

        // Initialize the backup time variables
        lastKnownGoodTime = initialTime;
        lastReadMillis = millis();
    }

    time_t nowUTC() {
        time_t rawTime = getRawRtcTime();

        // Sanity check: if time goes backwards, RTC has been reset or corrupted.
        if (rawTime < lastKnownGoodTime) {
            // Calculate elapsed time using the stable internal clock
            unsigned long elapsed = millis() - lastReadMillis;
            time_t correctedTime = lastKnownGoodTime + (elapsed / 1000);

            // Heal the RTC by writing the corrected time back to it
            rtc.setDS1302Time(::second(correctedTime), ::minute(correctedTime), ::hour(correctedTime), 0, ::day(correctedTime), ::month(correctedTime), ::year(correctedTime));
            
            // Update backups with the newly corrected time
            lastKnownGoodTime = correctedTime;
            lastReadMillis = millis();

            // Return the corrected time for this loop
            return correctedTime;
        }

        // If time is valid, update the backups and return the time
        lastKnownGoodTime = rawTime;
        lastReadMillis = millis();
        return rawTime;
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

        rtc.setDS1302Time(::second(utcTime), ::minute(utcTime), ::hour(utcTime), 0, ::day(utcTime), ::month(utcTime), ::year(utcTime));
        
        // Also update our backup time immediately
        lastKnownGoodTime = utcTime;
        lastReadMillis = millis();
    }

    void adjustTime(long adjustment, const Settings& settings) {
        time_t newTime = nowUTC() + adjustment; // nowUTC() will give a corrected time
        rtc.setDS1302Time(::second(newTime), ::minute(newTime), ::hour(newTime), 0, ::day(newTime), ::month(newTime), ::year(newTime));

        // Also update our backup time immediately
        lastKnownGoodTime = newTime;
        lastReadMillis = millis();
    }

private:
    virtuabotixRTC rtc;
    time_t lastKnownGoodTime = 0;
    unsigned long lastReadMillis = 0;

    // Helper to get raw time from RTC without any logic
    time_t getRawRtcTime() {
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
};

#endif // TIME_CONTROLLER_H
