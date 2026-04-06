#ifndef LIGHTING_CONTROLLER_H
#define LIGHTING_CONTROLLER_H

#include <TimeLib.h>
#include "Constants.h"
#include "Settings.h"
#include "Schedule.h"

class LightingController {
public:
    LightingController();
    void begin();
    void update(time_t now, const Settings& settings);

    float       getCurrentPowerPercent() const;
    float       getGlobalPowerPercent() const;
    const char* getCurrentPhaseName() const;
    uint8_t     getActiveBallastMask() const;
    bool        isSystemInFault() const;
    bool        isTransformerOn() const;

    bool        relaySwitched = false;

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
    float         powerBeforeTransition = 0.0f;

    bool          isFault = false;
    unsigned long faultCheckTimer = 0;

    int         outputVoltage = 0;

    bool          transformerOn = false;
    unsigned long transformerOnTime = 0;
    unsigned long lightsOffTime = 0;
    bool          cooldownActive = false;

    uint8_t     primaryPair = BALLAST_1;
    uint8_t     secondaryPair = BALLAST_2;
    int         lastRotationDay = -1;

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

    float getFeedbackVoltagePercent() const;
    void  regulateOutputVoltage();
};

#endif // LIGHTING_CONTROLLER_H
