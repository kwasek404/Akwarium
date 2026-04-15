#include "LightingController.h"
#include "TimeController.h"
#include <Arduino.h>

const unsigned long TRANSITION_STABILIZE_TIMEOUT = 3000;
const float TRANSITION_DIM_POWER = 5.0f;
const float STABILIZATION_THRESHOLD = 2.0f;

LightingController::LightingController() {
    currentBallastMask = 0;
}

void LightingController::begin(TimeController& tc) {
    timeCtrl = &tc;
    pinMode(VOLTAGE_OUTPUT_PIN, OUTPUT);
    pinMode(SWITCH_TRANSFORMER_PIN, OUTPUT);
    pinMode(SWITCH_BALLAST_1_PIN, OUTPUT);
    pinMode(SWITCH_BALLAST_2_PIN, OUTPUT);
    pinMode(SWITCH_BALLAST_3_PIN, OUTPUT);

    digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);
    digitalWrite(SWITCH_BALLAST_1_PIN, HIGH);
    digitalWrite(SWITCH_BALLAST_2_PIN, HIGH);
    digitalWrite(SWITCH_BALLAST_3_PIN, HIGH);
    analogWrite(VOLTAGE_OUTPUT_PIN, ANALOG_WRITE_RESOLUTION);
}

void LightingController::update(time_t now, const Settings& settings) {
    detectFaults();

    if (isFault) {
        scheduleTargetPower = 0;
        scheduleTargetBallastMask = 0;
    } else {
        updateDailyRotation(now);

        tmElements_t tm;
        breakTime(now, tm);
        long nowSeconds = tm.Hour * 3600L + tm.Minute * 60L + tm.Second;
        long startSeconds = settings.startHour * 3600L + settings.startMinute * 60L;
        long stopSeconds = settings.stopHour * 3600L + settings.stopMinute * 60L;
        if (stopSeconds <= startSeconds) {
            if (nowSeconds < startSeconds) { nowSeconds += 24 * 3600L; }
            stopSeconds += 24 * 3600L;
        }
        cachedNowSeconds = nowSeconds;
        cachedStartSeconds = startSeconds;
        cachedStopSeconds = stopSeconds;
        runScheduler(nowSeconds, startSeconds, stopSeconds);
    }

    manageTransformer();
    manageTransitions();
    regulateOutputVoltage();
}

bool LightingController::isTransformerOn() const { return transformerOn; }

float LightingController::getCurrentPowerPercent() const { return currentPowerPercent; }
const char* LightingController::getCurrentPhaseName() const { return currentPhaseName; }
uint8_t LightingController::getActiveBallastMask() const { return currentBallastMask; }
float LightingController::ballastOverhead(uint8_t mask) const {
    float overhead = 0.0f;
    if (mask & BALLAST_1) overhead += BALLAST_PAIR_OVERHEAD_WATTS;
    if (mask & BALLAST_2) overhead += BALLAST_PAIR_OVERHEAD_WATTS;
    if (mask & BALLAST_3) overhead += BALLAST_SINGLE_OVERHEAD_WATTS;
    return overhead;
}

float LightingController::getSystemWatts() const {
    int activeTubes = countTubesInMask(currentBallastMask);
    if (activeTubes == 0) return 0.0f;
    float dimFraction = currentPowerPercent / 100.0f;
    return ballastOverhead(currentBallastMask)
         + activeTubes * (CATHODE_WATTS_PER_TUBE + ARC_WATTS_PER_TUBE * dimFraction);
}

long LightingController::getSecondsToNextPhase() const {
    long totalDuration = cachedStopSeconds - cachedStartSeconds;
    if (totalDuration <= 0) return 0;

    switch (mainState) {
        case MainState::OFF: {
            if (cachedNowSeconds < cachedStartSeconds)
                return cachedStartSeconds - cachedNowSeconds;
            return 0;
        }
        case MainState::SIESTA: {
            long siestaEnd = cachedStartSeconds + (long)(totalDuration * SIESTA_END_PERCENT_OF_DAY);
            return max(0L, siestaEnd - cachedNowSeconds);
        }
        case MainState::MORNING_BLOCK:
        case MainState::EVENING_BLOCK:
            return max(0L, phaseEndSeconds - cachedNowSeconds);
        case MainState::FAULT:
        default:
            return 0;
    }
}

void LightingController::detectFaults() {
    if (scheduleTargetPower > 5.0f && getFeedbackVoltagePercent() < 1.0f) {
        if (faultCheckTimer == 0) {
            faultCheckTimer = millis();
        }
        if (millis() - faultCheckTimer > 5000) {
            isFault = true;
        }
    } else if (!isFault) {
        faultCheckTimer = 0;
    }
}

void LightingController::updateDailyRotation(time_t now) {
    tmElements_t tm;
    breakTime(now, tm);
    int today = tm.Day + tm.Month * 31;
    if (today == lastRotationDay) return;
    lastRotationDay = today;

    if (today % 2 == 0) {
        primaryPair = BALLAST_1;
        secondaryPair = BALLAST_2;
    } else {
        primaryPair = BALLAST_2;
        secondaryPair = BALLAST_1;
    }
}

uint8_t LightingController::selectOptimalMask(float systemPower, bool isMorning) const {
    if (systemPower <= 0.0f) return 0;

    if (isMorning) {
        // Morning: B3 solo up to 20%, then B3 + primaryPair
        if (systemPower <= 20.0f) {
            return BALLAST_3;
        }
        return BALLAST_3 | primaryPair;
    } else {
        // Evening: primaryPair solo up to 40%, then all 5 tubes
        if (systemPower <= 40.0f) {
            return primaryPair;
        }
        return primaryPair | secondaryPair | BALLAST_3;
    }
}

void LightingController::runScheduler(long nowSeconds, long startSeconds, long stopSeconds) {
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
        case MainState::FAULT:
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
                               PRO_SCHEDULE_MORNING, PRO_SCHEDULE_MORNING_PHASES_COUNT,
                               nowSeconds, true);
            break;
        case MainState::EVENING_BLOCK:
            processActiveBlock(siestaEndSeconds, stopSeconds - siestaEndSeconds,
                               PRO_SCHEDULE_EVENING, PRO_SCHEDULE_EVENING_PHASES_COUNT,
                               nowSeconds, false);
            break;
    }
}

void LightingController::processActiveBlock(long blockStartSeconds, long blockDuration,
                                            const SchedulePhase* phases, int phaseCount,
                                            long nowSeconds, bool isMorning) {
    if (blockDuration <= 0) return;
    float blockProgress = (float)(nowSeconds - blockStartSeconds) / blockDuration;

    for (int i = 0; i < phaseCount; i++) {
        const SchedulePhase& phase = phases[i];
        if (blockProgress >= phase.startPercent && blockProgress <= phase.endPercent) {
            currentPhaseName = phase.name;

            float scheduleTotalSystemPower;
            if (phase.type == PhaseType::HOLD) {
                scheduleTotalSystemPower = phase.startPower;
            } else {
                float phaseDurationPercent = phase.endPercent - phase.startPercent;
                float progress = (phaseDurationPercent <= 0) ? 1.0f : (blockProgress - phase.startPercent) / phaseDurationPercent;
                float rampProgress = progress;

                if (phase.type == PhaseType::RAMP_QUAD_IN) { rampProgress = progress * progress; }
                else if (phase.type == PhaseType::RAMP_QUAD_OUT) { rampProgress = 1.0f - (1.0f - progress) * (1.0f - progress); }
                scheduleTotalSystemPower = phase.startPower + (phase.endPower - phase.startPower) * rampProgress;
            }

            scheduleTargetBallastMask = selectOptimalMask(scheduleTotalSystemPower, isMorning);

            int tubesInMask = countTubesInMask(scheduleTargetBallastMask);
            if (tubesInMask > 0) {
                float perTubePower = (scheduleTotalSystemPower * 5.0f) / tubesInMask;
                scheduleTargetPower = constrain(perTubePower, 0.0f, 100.0f);
            } else {
                scheduleTargetPower = 0;
            }

            phaseEndSeconds = blockStartSeconds + (long)(phase.endPercent * blockDuration);
            return;
        }
    }
}

void LightingController::manageTransitions() {
    if (transitionState == TransitionState::IDLE) {
        if (currentBallastMask != scheduleTargetBallastMask) {
            transitionState = TransitionState::START_TRANSITION;
        } else {
            targetPowerPercent = scheduleTargetPower;
        }
        return;
    }

    unsigned long elapsed = millis() - transitionStartTime;

    switch(transitionState) {
        case TransitionState::START_TRANSITION:
            powerBeforeTransition = currentPowerPercent;
            targetPowerPercent = TRANSITION_DIM_POWER;
            transitionStartTime = millis();
            stabilityCounter = 0;
            transitionState = TransitionState::WAIT_FOR_DIM;
            break;

        case TransitionState::WAIT_FOR_DIM:
            if (abs(currentPowerPercent - targetPowerPercent) < STABILIZATION_THRESHOLD) {
                stabilityCounter++;
                if (stabilityCounter >= STABILITY_REQUIRED_COUNT) {
                    stabilityCounter = 0;
                    transitionState = TransitionState::SWITCH_BALLAST;
                }
            } else {
                stabilityCounter = 0;
            }
            if (elapsed > TRANSITION_STABILIZE_TIMEOUT) {
                stabilityCounter = 0;
                transitionState = TransitionState::SWITCH_BALLAST;
            }
            break;

        case TransitionState::SWITCH_BALLAST: {
            uint8_t to_add = scheduleTargetBallastMask & ~currentBallastMask;
            uint8_t to_remove = currentBallastMask & ~scheduleTargetBallastMask;
            int oldTubes = countTubesInMask(currentBallastMask);
            uint8_t nextMask = currentBallastMask;

            if (to_add) {
                uint8_t add = (to_add & BALLAST_1) ? BALLAST_1 : ((to_add & BALLAST_2) ? BALLAST_2 : BALLAST_3);
                nextMask |= add;
            } else if (to_remove) {
                uint8_t remove = (to_remove & BALLAST_3) ? BALLAST_3 : ((to_remove & BALLAST_2) ? BALLAST_2 : BALLAST_1);
                nextMask &= ~remove;
            }

            int newTubes = countTubesInMask(nextMask);
            setBallasts(nextMask);

            if (newTubes > 0) {
                float compensatedPower;
                if (oldTubes > 0) {
                    compensatedPower = (powerBeforeTransition * oldTubes) / newTubes;
                } else {
                    compensatedPower = scheduleTargetPower;
                }
                 targetPowerPercent = constrain(compensatedPower, 0.0f, 100.0f);
            } else {
                targetPowerPercent = 0;
            }

            transitionState = TransitionState::RAMP_UP;
            transitionStartTime = millis();
            lastBallastSwitchTime = millis();
            break;
        }

        case TransitionState::RAMP_UP:
            targetPowerPercent = scheduleTargetPower;
            transitionState = TransitionState::WAIT_FOR_BRIGHT;
            transitionStartTime = millis();
            break;

        case TransitionState::WAIT_FOR_BRIGHT:
            if (abs(currentPowerPercent - targetPowerPercent) < STABILIZATION_THRESHOLD) {
                stabilityCounter++;
                if (stabilityCounter >= STABILITY_REQUIRED_COUNT) {
                    stabilityCounter = 0;
                    transitionState = TransitionState::FINISH_TRANSITION;
                }
            } else {
                stabilityCounter = 0;
            }
            if (elapsed > TRANSITION_STABILIZE_TIMEOUT) {
                stabilityCounter = 0;
                transitionState = TransitionState::FINISH_TRANSITION;
            }
            break;

        case TransitionState::FINISH_TRANSITION:
            if (currentBallastMask != scheduleTargetBallastMask) {
                if (millis() - lastBallastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS) {
                    transitionState = TransitionState::START_TRANSITION;
                }
            } else {
                transitionState = TransitionState::IDLE;
            }
            break;

        case TransitionState::IDLE: break;
    }
}

void LightingController::setBallasts(uint8_t mask) {
    if (currentBallastMask == mask) return;
    digitalWrite(SWITCH_BALLAST_1_PIN, (mask & BALLAST_1) ? LOW : HIGH);
    digitalWrite(SWITCH_BALLAST_2_PIN, (mask & BALLAST_2) ? LOW : HIGH);
    digitalWrite(SWITCH_BALLAST_3_PIN, (mask & BALLAST_3) ? LOW : HIGH);
    if (timeCtrl) timeCtrl->suppressReads(500);
    currentBallastMask = mask;
}

int LightingController::countTubesInMask(uint8_t mask) const {
    int count = 0;
    if (mask & BALLAST_1) count += 2;
    if (mask & BALLAST_2) count += 2;
    if (mask & BALLAST_3) count += 1;
    return count;
}

float LightingController::getFeedbackVoltagePercent() const {
    float measuredVoltage = (analogRead(VOLTAGE_FEEDBACK_PIN) / (float)ANALOG_READ_RESOLUTION) * 5.0f * 2.0f;
    float power = (measuredVoltage - 1.0f) * (100.0f / 9.0f);
    return constrain(power, 0.0f, 100.0f);
}

void LightingController::manageTransformer() {
    bool lightsNeeded = (scheduleTargetBallastMask != 0) || (scheduleTargetPower > 0);

    if (lightsNeeded) {
        cooldownActive = false;
        if (!transformerOn) {
            digitalWrite(SWITCH_TRANSFORMER_PIN, LOW);
            transformerOn = true;
            transformerOnTime = millis();
            relaySwitched = true;
        }
    } else {
        if (transformerOn && !cooldownActive) {
            cooldownActive = true;
            lightsOffTime = millis();
        }
        if (cooldownActive && (millis() - lightsOffTime >= FAN_COOLDOWN_MS)) {
            digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);
            transformerOn = false;
            cooldownActive = false;
            relaySwitched = true;
        }
    }
}

void LightingController::regulateOutputVoltage() {
    currentPowerPercent = getFeedbackVoltagePercent();
    if (mainState == MainState::OFF || mainState == MainState::FAULT || !transformerOn) {
        targetPowerPercent = 0;
    }

    if (transformerOn && (millis() - transformerOnTime < TRANSFORMER_WARMUP_MS)) {
        targetPowerPercent = 0;
        return;
    }

    float error = targetPowerPercent - currentPowerPercent;
    outputVoltage += (int)(error * VOLTAGE_KP);
    outputVoltage = constrain(outputVoltage, 0, ANALOG_WRITE_RESOLUTION);
    analogWrite(VOLTAGE_OUTPUT_PIN, ANALOG_WRITE_RESOLUTION - outputVoltage);
}
