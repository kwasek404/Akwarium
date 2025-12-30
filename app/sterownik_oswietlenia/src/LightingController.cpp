#include "LightingController.h"
#include <Arduino.h>

// --- Constants for transition logic ---
const unsigned long TRANSITION_RAMP_DURATION = 2000; // 2 seconds for each ramp
const unsigned long TRANSITION_STABILIZE_TIMEOUT = 3000; // 3 seconds to wait for stabilization
const float TRANSITION_DIM_POWER = 5.0f; // Dim to 5% power before switching
const float STABILIZATION_THRESHOLD = 2.0f; // Power is stable if within 2% of target

//
// --- Public Functions ---
//

LightingController::LightingController() {
    pinMode(VOLTAGE_OUTPUT_PIN, OUTPUT);
    pinMode(SWITCH_TRANSFORMER_PIN, OUTPUT);
    pinMode(SWITCH_BALLAST_1_PIN, OUTPUT);
    pinMode(SWITCH_BALLAST_2_PIN, OUTPUT);
    pinMode(SWITCH_BALLAST_3_PIN, OUTPUT);

    digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);
    digitalWrite(SWITCH_BALLAST_1_PIN, HIGH);
    digitalWrite(SWITCH_BALLAST_2_PIN, HIGH);
    digitalWrite(SWITCH_BALLAST_3_PIN, HIGH);
    currentBallastMask = 0;

    analogWrite(VOLTAGE_OUTPUT_PIN, ANALOG_WRITE_RESOLUTION);
}

void LightingController::begin() {
    digitalWrite(SWITCH_TRANSFORMER_PIN, LOW);
}

void LightingController::update(time_t now, const Settings& settings) {
    // Layer 1: Fault Detection (highest priority)
    detectFaults();
    if (isFault) {
        targetPowerPercent = 0;
        scheduleTargetBallastMask = 0;
        currentPhaseName = "Fault";
    } else {
        // Layer 2: Scheduler (determines ultimate targets)
        tmElements_t tm;
        breakTime(now, tm);
        long nowSeconds = tm.Hour * 3600L + tm.Minute * 60L + tm.Second;
        long startSeconds = settings.startHour * 3600L + settings.startMinute * 60L;
        long stopSeconds = settings.stopHour * 3600L + settings.stopMinute * 60L;
        if (stopSeconds <= startSeconds) {
            if (nowSeconds < startSeconds) { nowSeconds += 24 * 3600L; }
            stopSeconds += 24 * 3600L;
        }
        runStateMachine(nowSeconds, startSeconds, stopSeconds);
    }

    // Layer 3: Transition Manager (determines immediate targets)
    manageTransitions();

    // Layer 4: Feedback Controller (executes immediate targets)
    regulateOutputVoltage();
}

float LightingController::getCurrentPowerPercent() const { return currentPowerPercent; }
const char* LightingController::getCurrentPhaseName() const { return currentPhaseName; }
uint8_t LightingController::getActiveBallastMask() const { return currentBallastMask; }
bool LightingController::isSystemInFault() const { return isFault; }

//
// --- Private Methods ---
//

void LightingController::detectFaults() {
    // Fault: We are commanding power, but getting no feedback voltage.
    if (targetPowerPercent > 5.0f && getFeedbackVoltagePercent() < 1.0f) {
        if (faultCheckTimer == 0) {
            faultCheckTimer = millis();
        }
        if (millis() - faultCheckTimer > 5000) { // 5 second timeout
            isFault = true;
        }
    } else {
        faultCheckTimer = 0; // Reset timer if conditions are normal
        isFault = false;
    }
}

void LightingController::runStateMachine(long nowSeconds, long startSeconds, long stopSeconds) {
    long totalDuration = stopSeconds - startSeconds;
    if (totalDuration <= 0) { mainState = MainState::OFF; }

    long siestaStartSeconds = startSeconds + totalDuration * SIESTA_START_PERCENT_OF_DAY;
    long siestaEndSeconds = startSeconds + totalDuration * SIESTA_END_PERCENT_OF_DAY;

    if (nowSeconds < startSeconds || nowSeconds >= stopSeconds) {
        mainState = MainState::OFF;
    } else if (nowSeconds >= siestaStartSeconds && nowSeconds < siestaEndSeconds) {
        mainState = MainState::SIESTA;
    } else if (nowSeconds < siestaStartSeconds) {
        mainState = MainState::MORNING_BLOCK;
    } else {
        mainState = MainState::EVENING_BLOCK;
    }

    switch (mainState) {
        case MainState::OFF:
        case MainState::FAULT: // Treat fault like OFF
            scheduleTargetPower = 0;
            scheduleTargetBallastMask = 0;
            currentPhaseName = (mainState == MainState::FAULT) ? "Fault" : "Off";
            break;
        case MainState::SIESTA:
            scheduleTargetPower = 0;
            scheduleTargetBallastMask = 0;
            currentPhaseName = "Siesta";
            break;
        case MainState::MORNING_BLOCK:
            processActiveBlock(startSeconds, siestaStartSeconds - startSeconds,
                               PRO_SCHEDULE_MORNING, PRO_SCHEDULE_MORNING_PHASES_COUNT, nowSeconds);
            break;
        case MainState::EVENING_BLOCK:
            processActiveBlock(siestaEndSeconds, stopSeconds - siestaEndSeconds,
                               PRO_SCHEDULE_EVENING, PRO_SCHEDULE_EVENING_PHASES_COUNT, nowSeconds);
            break;
    }
}

void LightingController::processActiveBlock(long blockStartSeconds, long blockDuration, const SchedulePhase* phases, int phaseCount, long nowSeconds) {
    if (blockDuration <= 0) return;
    float blockProgress = (float)(nowSeconds - blockStartSeconds) / blockDuration;

    for (int i = 0; i < phaseCount; i++) {
        const SchedulePhase& phase = phases[i];
        if (blockProgress >= phase.startPercent && blockProgress <= phase.endPercent) {
            currentPhaseName = phase.name;
            scheduleTargetBallastMask = phase.ballastMask;

            float phaseDurationPercent = phase.endPercent - phase.startPercent;
            float progress = (phaseDurationPercent <= 0) ? 1.0f : (blockProgress - phase.startPercent) / phaseDurationPercent;
            float rampProgress = progress;

            if (phase.type == PhaseType::HOLD) {
                rampProgress = 1.0;
            } else if (phase.type == PhaseType::RAMP_QUAD_IN) {
                rampProgress = progress * progress;
            } else if (phase.type == PhaseType::RAMP_QUAD_OUT) {
                rampProgress = 1.0f - (1.0f - progress) * (1.0f - progress);
            }
            scheduleTargetPower = phase.startPower + (phase.endPower - phase.startPower) * rampProgress;
            return;
        }
    }
}

void LightingController::manageTransitions() {
    if (transitionState == TransitionState::IDLE) {
        if (currentBallastMask != scheduleTargetBallastMask) {
            transitionState = TransitionState::RAMP_DOWN; // Start the process
        } else {
            // Not transitioning, so the immediate target is the schedule's target.
            targetPowerPercent = scheduleTargetPower;
        }
        return;
    }

    unsigned long elapsed = millis() - transitionStartTime;

    switch(transitionState) {
        case TransitionState::RAMP_DOWN:
            targetPowerPercent = TRANSITION_DIM_POWER;
            transitionStartTime = millis();
            transitionState = TransitionState::WAIT_FOR_DIM;
            break;

        case TransitionState::WAIT_FOR_DIM:
            // Wait for the feedback loop to bring the power down
            if (abs(currentPowerPercent - targetPowerPercent) < STABILIZATION_THRESHOLD || elapsed > TRANSITION_STABILIZE_TIMEOUT) {
                transitionState = TransitionState::SWITCH_BALLAST;
            }
            break;

        case TransitionState::SWITCH_BALLAST:
            { // new scope for local variables
                uint8_t to_add = scheduleTargetBallastMask & ~currentBallastMask;
                uint8_t to_remove = currentBallastMask & ~scheduleTargetBallastMask;
                
                if (to_add != 0) {
                    uint8_t ballast_to_add = (to_add & BALLAST_1) ? BALLAST_1 : ((to_add & BALLAST_2) ? BALLAST_2 : BALLAST_3);
                    setBallasts(currentBallastMask | ballast_to_add);
                } else if (to_remove != 0) {
                    uint8_t ballast_to_remove = (to_remove & BALLAST_3) ? BALLAST_3 : ((to_remove & BALLAST_2) ? BALLAST_2 : BALLAST_1);
                    setBallasts(currentBallastMask & ~ballast_to_remove);
                }
                
                transitionState = TransitionState::RAMP_UP;
                transitionStartTime = millis();
                lastBallastSwitchTime = millis();
            }
            break;

        case TransitionState::RAMP_UP:
            targetPowerPercent = scheduleTargetPower;
            transitionState = TransitionState::WAIT_FOR_BRIGHT;
            transitionStartTime = millis();
            break;

        case TransitionState::WAIT_FOR_BRIGHT:
            // Wait for the feedback loop to bring the power up
            if (abs(currentPowerPercent - targetPowerPercent) < STABILIZATION_THRESHOLD || elapsed > TRANSITION_STABILIZE_TIMEOUT) {
                transitionState = TransitionState::FINISH_TRANSITION;
            }
            break;
            
        case TransitionState::FINISH_TRANSITION:
            // One ballast done. Check if more are needed.
            if (currentBallastMask != scheduleTargetBallastMask) {
                // More to do, wait a second before starting again
                if(millis() - lastBallastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS) {
                     transitionState = TransitionState::RAMP_DOWN;
                }
            } else {
                // All transitions complete, go back to idle
                transitionState = TransitionState::IDLE;
            }
            break;

        case TransitionState::IDLE:
             // Should not happen here, but handle it
             break;
        
        // This state is handled by the initial check in this function
        case TransitionState::START_TRANSITION:
             transitionState = TransitionState::IDLE;
             break;
    }
}

void LightingController::setBallasts(uint8_t mask) {
    if (currentBallastMask == mask) return;
    digitalWrite(SWITCH_BALLAST_1_PIN, (mask & BALLAST_1) ? LOW : HIGH);
    digitalWrite(SWITCH_BALLAST_2_PIN, (mask & BALLAST_2) ? LOW : HIGH);
    digitalWrite(SWITCH_BALLAST_3_PIN, (mask & BALLAST_3) ? LOW : HIGH);
    currentBallastMask = mask;
}

float LightingController::getFeedbackVoltagePercent() const {
    float measuredVoltage = (analogRead(VOLTAGE_FEEDBACK_PIN) / (float)ANALOG_READ_RESOLUTION) * 5.0f * 2.0f;
    float power = (measuredVoltage - 1.0f) * (100.0f / 9.0f);
    return constrain(power, 0.0f, 100.0f);
}

void LightingController::regulateOutputVoltage() {
    currentPowerPercent = getFeedbackVoltagePercent();
    if (mainState == MainState::OFF || mainState == MainState::FAULT) {
        targetPowerPercent = 0;
    }
    
    float error = targetPowerPercent - currentPowerPercent;
    outputVoltage += (int)(error * VOLTAGE_KP);
    outputVoltage = constrain(outputVoltage, 0, ANALOG_WRITE_RESOLUTION);
    analogWrite(VOLTAGE_OUTPUT_PIN, ANALOG_WRITE_RESOLUTION - outputVoltage);
}