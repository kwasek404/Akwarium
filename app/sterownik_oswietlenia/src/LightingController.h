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

    // --- Public Getters for UI ---
    float       getCurrentPowerPercent() const;
    const char* getCurrentPhaseName() const;
    uint8_t     getActiveBallastMask() const;
    bool        isSystemInFault() const;

private:
    // --- High-Level State Machine ---
    enum class MainState {
        OFF,
        MORNING_BLOCK,
        SIESTA,
        EVENING_BLOCK,
        FAULT
    };
    MainState mainState = MainState::OFF;

    // --- Ballast Transition Sub-State Machine ---
    enum class TransitionState {
        IDLE,
        START_TRANSITION,
        RAMP_DOWN,
        WAIT_FOR_DIM,
        SWITCH_BALLAST,
        RAMP_UP,
        WAIT_FOR_BRIGHT,
        FINISH_TRANSITION
    };
    TransitionState transitionState = TransitionState::IDLE;

    // --- Core State & Target Variables ---
    const char* currentPhaseName = "Off";
    float       currentPowerPercent = 0.0f;     // Actual power from feedback
    uint8_t     currentBallastMask = 0;
    float       targetPowerPercent = 0.0f;      // The immediate power target for the feedback loop
    uint8_t     scheduleTargetBallastMask = 0;  // The ultimate mask target from the schedule
    float       scheduleTargetPower = 0.0f;     // The ultimate power target from the schedule
    
    // --- State Timers & Variables ---
    unsigned long lastBallastSwitchTime = 0;
    unsigned long transitionStartTime = 0;
    float         powerBeforeTransition = 0.0f;
    
    // --- Fault Detection ---
    bool          isFault = false;
    unsigned long faultCheckTimer = 0;

    // --- Feedback Loop Controller State ---
    int         outputVoltage = 0;

    // --- Private Methods ---
    void detectFaults();
    void runScheduler(long nowSeconds, long startSeconds, long stopSeconds);
    void processActiveBlock(long blockStartSeconds, long blockDuration, const SchedulePhase* phases, int phaseCount, long nowSeconds);
    void manageTransitions();
    void setBallasts(uint8_t mask);
    int  countTubesInMask(uint8_t mask) const;
    
    float getFeedbackVoltagePercent() const;
    void  regulateOutputVoltage();
};

#endif // LIGHTING_CONTROLLER_H
