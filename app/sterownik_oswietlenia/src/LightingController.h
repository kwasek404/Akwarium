#ifndef LIGHTING_CONTROLLER_H
#define LIGHTING_CONTROLLER_H

#include <TimeLib.h>
#include "Constants.h"
#include "Settings.h"
#include "Schedule.h"

class TimeController;

class LightingController {
public:
    LightingController();
    void begin(TimeController& tc);
    void update(time_t now, const Settings& settings);

    float       getCurrentPowerPercent() const;
    float       getSystemWatts() const;
    long        getSecondsToNextPhase() const;
    const char* getCurrentPhaseName() const;
    uint8_t     getActiveBallastMask() const;
    bool        isSystemInFault() const;
    bool        isTransformerOn() const;
    void        triggerSoftStart();

    bool        relaySwitched = false;
    bool        overrideEnabled = false;
    uint8_t     overridePowerPercent = 0;

private:
    enum class MainState {
        OFF,
        MORNING_BLOCK,
        SIESTA,
        EVENING_BLOCK,
        FAULT
    };
    MainState mainState = MainState::OFF;

    enum class TransitionState {
        IDLE,
        START_TRANSITION,
        WAIT_FOR_DIM,
        SWITCH_BALLAST,
        RAMP_UP,
        WAIT_FOR_BRIGHT,
        FINISH_TRANSITION
    };
    TransitionState transitionState = TransitionState::IDLE;

    const char* currentPhaseName = "Off";
    float       currentPowerPercent = 0.0f;
    uint8_t     currentBallastMask = 0;
    float       targetPowerPercent = 0.0f;
    uint8_t     scheduleTargetBallastMask = 0;
    float       scheduleTargetPower = 0.0f;

    unsigned long lastBallastSwitchTime = 0;
    unsigned long transitionStartTime = 0;

    bool          isFault = false;
    unsigned long faultCheckTimer = 0;

    float       outputPercent = 0.0f; // 0-100%, converted to PWM only at analogWrite

    bool          transformerOn = false;
    unsigned long transformerOnTime = 0;
    unsigned long lightsOffTime = 0;
    bool          cooldownActive = false;

    uint8_t     primaryPair = BALLAST_1;
    uint8_t     secondaryPair = BALLAST_2;
    int         lastRotationDay = -1;

    long        phaseEndSeconds = 0;
    long        cachedNowSeconds = 0;
    long        cachedStartSeconds = 0;
    long        cachedStopSeconds = 0;

    TimeController* timeCtrl = nullptr;
    unsigned long stabilityWindowStart = 0;

    bool          softStartActive = false;
    unsigned long softStartBeginMs = 0;
    bool          firstUpdate = true;

    void detectFaults();
    void runScheduler(long nowSeconds, long startSeconds, long stopSeconds);
    void processActiveBlock(long blockStartSeconds, long blockDuration,
                            const SchedulePhase* phases, int phaseCount,
                            long nowSeconds, bool isMorning);
    void updateDailyRotation(time_t now);
    uint8_t selectOptimalMask(float systemPower, bool isMorning) const;
    void manageTransitions();
    void manageTransformer();
    void setBallasts(uint8_t mask);
    int  countTubesInMask(uint8_t mask) const;

    float ballastOverhead(uint8_t mask) const;
    float getFeedbackVoltagePercent() const;
    void  regulateOutputVoltage();
};

#endif // LIGHTING_CONTROLLER_H
