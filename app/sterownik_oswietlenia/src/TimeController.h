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

    uint8_t bootValidCount = 0;
    uint8_t bootQuorumSize = 0;
    bool bootQuorumReached = false;
    uint16_t runtimeBadReads = 0;

    void suppressReads(unsigned long durationMs) {
        unsigned long until = millis() + durationMs;
        if (until > suppressUntil) {
            suppressUntil = until;
        }
    }

    void begin() {
        rtc.begin();

        const uint8_t NUM_READS = 7;
        time_t readings[NUM_READS];
        for (uint8_t i = 0; i < NUM_READS; i++) {
            readings[i] = getRawRtcTime();
            if (i < NUM_READS - 1) delay(30);
        }

        uint8_t validCount = 0;
        for (uint8_t i = 0; i < NUM_READS; i++) {
            if (isTimeValid(readings[i])) validCount++;
        }
        bootValidCount = validCount;

        time_t initialTime = getQuorumTime(readings, NUM_READS);

        ::setTime(initialTime);
        lastKnownGoodTime = initialTime;
        lastSyncMillis = millis();
    }

    time_t nowUTC() {
        unsigned long now = millis();

        bool suppressed = (suppressUntil != 0 && now < suppressUntil);
        if (suppressed) {
            return lastKnownGoodTime + (now - lastSyncMillis) / 1000;
        }
        suppressUntil = 0;

        if (now - lastSyncMillis < RTC_SYNC_INTERVAL_MS) {
            return lastKnownGoodTime + (now - lastSyncMillis) / 1000;
        }

        time_t rawTime = getRawRtcTime();

        if (isTimeValid(rawTime)) {
            lastKnownGoodTime = rawTime;
            lastSyncMillis = now;
            return rawTime;
        }

        runtimeBadReads++;
        return lastKnownGoodTime + (now - lastSyncMillis) / 1000;
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

        lastKnownGoodTime = utcTime;
        lastSyncMillis = millis();
    }

    void adjustTime(long adjustment, const Settings& settings) {
        time_t newTime = nowUTC() + adjustment;
        rtc.setDS1302Time(::second(newTime), ::minute(newTime), ::hour(newTime), 0, ::day(newTime), ::month(newTime), ::year(newTime));

        lastKnownGoodTime = newTime;
        lastSyncMillis = millis();
    }

private:
    virtuabotixRTC rtc;
    time_t lastKnownGoodTime = 0;
    unsigned long lastSyncMillis = 0;
    unsigned long suppressUntil = 0;

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

    bool isTimeValid(time_t t) {
        if (t <= 0) return false;
        int y = ::year(t);
        int m = ::month(t);
        int d = ::day(t);
        int h = ::hour(t);
        int mi = ::minute(t);
        int s = ::second(t);
        if (m < 1 || m > 12) return false;
        if (d < 1 || d > 31) return false;
        if (h > 23) return false;
        if (mi > 59) return false;
        if (s > 59) return false;
        if (y < 2000 || y > 2099) return false;
        return true;
    }

    time_t getQuorumTime(time_t* readings, uint8_t count) {
        uint8_t bestCount = 0;
        time_t bestTime = readings[0];

        for (uint8_t i = 0; i < count; i++) {
            if (!isTimeValid(readings[i])) continue;
            uint8_t matches = 0;
            for (uint8_t j = 0; j < count; j++) {
                if (!isTimeValid(readings[j])) continue;
                long diff = (long)(readings[j] - readings[i]);
                if (diff < 0) diff = -diff;
                if (diff <= 2) matches++;
            }
            if (matches > bestCount) {
                bestCount = matches;
                bestTime = readings[i];
            }
        }

        bootQuorumSize = bestCount;
        bootQuorumReached = (bestCount >= 3);

        if (bestCount >= 3) return bestTime;

        for (uint8_t i = 0; i < count; i++) {
            if (isTimeValid(readings[i])) return readings[i];
        }

        return readings[count - 1];
    }
};

#endif // TIME_CONTROLLER_H
