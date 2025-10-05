#ifndef LIGHTING_CONTROLLER_H
#define LIGHTING_CONTROLLER_H

#include "Debug.h"
#include "Constants.h"
#include "Settings.h"
#include <TimeLib.h>

class LightingController {
private:
    static constexpr float MIN_POWER_PERCENT = 1.0f; // Minimum power before shutting down

public:
    LightingController() {
        pinMode(VOLTAGE_OUTPUT_PIN, OUTPUT);
        pinMode(VOLTAGE_FEEDBACK_PIN, INPUT);
        pinMode(SWITCH_TRANSFORMER_PIN, OUTPUT);
        pinMode(SWITCH_BALLAST_1_PIN, OUTPUT);
        pinMode(SWITCH_BALLAST_2_PIN, OUTPUT);
        pinMode(SWITCH_BALLAST_3_PIN, OUTPUT);
        
        // Initialize all switches to OFF (HIGH for relays)
        digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);
        digitalWrite(SWITCH_BALLAST_1_PIN, HIGH);
        digitalWrite(SWITCH_BALLAST_2_PIN, HIGH);
        digitalWrite(SWITCH_BALLAST_3_PIN, HIGH);
    }

    void update(time_t now, const Settings& settings) {
        calculateTargetPower(now, settings);
        manageSwitches();
        regulateOutputVoltage();
    }

    float getCurrentPowerPercent() const { return currentPowerPercent; }
    float getFeedbackVoltagePercent() const {
        // Assuming a 5V ADC reference and a voltage divider that scales 0-10V from the lamp to 0-5V for the Arduino.
        // So, the actual voltage is (analogRead / 1023) * 5V * 2.
        float measuredVoltage = (analogRead(VOLTAGE_FEEDBACK_PIN) / (float)ANALOG_READ_RESOLUTION) * 5.0f * 2.0f;

        // The formula y = 11.11x - 11.11 maps 1V->0% and 10V->100%.
        // Let's adjust it slightly to handle the 1-10V range mapping to 0-100% power.
        // If 1V -> 0%, 10V -> 100%, then Power = (Voltage - 1) * (100 / 9)
        float power = (measuredVoltage - 1.0f) * (100.0f / 9.0f);

        return constrain(power, 0.0f, 100.0f);
    }

    bool isSynchronizing() const {
        // Consider it synchronizing if the difference between target and actual power is significant
        float error = targetPowerPercent - getFeedbackVoltagePercent();
        return abs(error) > 2.0f; // 2% threshold for stabilization
    }

private:
    enum class State { OFF, STARTING, STABILIZING, RUNNING, STOPPING };
    State state = State::OFF;
    
    float targetPowerPercent = 0.0f;
    float lastLoggedTarget = -1.0f;
    float currentPowerPercent = 0.0f;
    int outputVoltage = 0;
    unsigned long lastSwitchTime = 0;
    int ballastsOn = 0;

    void calculateTargetPower(time_t now, const Settings& settings) {
        tmElements_t tm;
        breakTime(now, tm);
        long nowSeconds = tm.Hour * 3600L + tm.Minute * 60L + tm.Second;

        long startSeconds = settings.startHour * 3600L + settings.startMinute * 60L;
        long stopSeconds = settings.stopHour * 3600L + settings.stopMinute * 60L;

        if (startSeconds >= stopSeconds) { // Overnight case
            if (nowSeconds < startSeconds && nowSeconds > stopSeconds) {
                targetPowerPercent = 0.0f;
                return;
            }
            if (nowSeconds >= startSeconds) {
                stopSeconds += 24 * 3600L;
            } else { // nowSeconds < stopSeconds
                startSeconds -= 24 * 3600L;
            }
        } else { // Same day case
            if (nowSeconds < startSeconds || nowSeconds > stopSeconds) {
                targetPowerPercent = 0.0f;
                return;
            }
        }

        long duration = stopSeconds - startSeconds;
        if (duration <= 0) {
            targetPowerPercent = 0.0f;
            return;
        }

        long elapsed = nowSeconds - startSeconds;
        float phase = (float)elapsed / duration; // 0.0 to 1.0

        // Sinusoidal curve for natural lighting
        targetPowerPercent = sin(phase * PI) * 100.0f;

        // Clamp to valid range and handle shutdown threshold
        if (targetPowerPercent < MIN_POWER_PERCENT) {
            targetPowerPercent = 0.0f;
        } else {
            targetPowerPercent = constrain(targetPowerPercent, 0.0f, 100.0f);
        }

        if (abs(targetPowerPercent - lastLoggedTarget) > 0.5f) {
            DEBUG_PRINTF("Lighting: New target power: %.2f%%\n", targetPowerPercent);
            lastLoggedTarget = targetPowerPercent;
        }
    }

    void manageSwitches() {
        bool shouldBeOn = targetPowerPercent >= MIN_POWER_PERCENT;
        unsigned long currentTime = millis();

        switch (state) {
            case State::OFF:
                if (shouldBeOn) {
                    DEBUG_PRINTLN("Lighting: State OFF -> STARTING");
                    state = State::STARTING;
                    lastSwitchTime = currentTime;
                    digitalWrite(SWITCH_TRANSFORMER_PIN, LOW); // Turn on transformer
                }
                break;

            case State::STARTING:
                // Wait for transformer to power up before stabilizing voltage
                if (currentTime - lastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS) {
                    DEBUG_PRINTLN("Lighting: State STARTING -> STABILIZING");
                    state = State::STABILIZING;
                }
                break;

            case State::STABILIZING:
                if (!shouldBeOn) {
                    DEBUG_PRINTLN("Lighting: State STABILIZING -> STOPPING");
                    state = State::STOPPING;
                    lastSwitchTime = currentTime;
                    break;
                }
                // Wait for voltage to stabilize before turning on ballasts
                if (!isSynchronizing()) {
                    DEBUG_PRINTLN("Lighting: State STABILIZING -> RUNNING (Voltage stabilized)");
                    state = State::RUNNING;
                    lastSwitchTime = currentTime; // Reset timer for ballast switching
                }
                break;

            case State::RUNNING:
                if (!shouldBeOn) {
                    DEBUG_PRINTLN("Lighting: State RUNNING -> STOPPING");
                    state = State::STOPPING;
                    lastSwitchTime = currentTime;
                    break;
                }
                // Sequentially turn on ballasts
                if (ballastsOn < 3 && (currentTime - lastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS)) {
                    DEBUG_PRINTF("Lighting: Turning on ballast #%d\n", ballastsOn + 1);
                    if (ballastsOn == 0) digitalWrite(SWITCH_BALLAST_1_PIN, LOW);
                    if (ballastsOn == 1) digitalWrite(SWITCH_BALLAST_2_PIN, LOW);
                    if (ballastsOn == 2) digitalWrite(SWITCH_BALLAST_3_PIN, LOW);
                    ballastsOn++;
                    lastSwitchTime = currentTime;
                }
                break;

            case State::STOPPING:
                if (currentTime - lastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS) {
                    if (ballastsOn > 0) {
                        // Sequentially turn off ballasts
                        DEBUG_PRINTF("Lighting: Turning off ballast #%d\n", ballastsOn);
                        if (ballastsOn == 3) digitalWrite(SWITCH_BALLAST_3_PIN, HIGH);
                        if (ballastsOn == 2) digitalWrite(SWITCH_BALLAST_2_PIN, HIGH);
                        if (ballastsOn == 1) digitalWrite(SWITCH_BALLAST_1_PIN, HIGH);
                        ballastsOn--;
                        lastSwitchTime = currentTime;
                    }
                } else if (ballastsOn == 0) {
                    // All ballasts are off, now turn off transformer
                    DEBUG_PRINTLN("Lighting: All ballasts off. Turning off transformer. State -> OFF");
                    digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);
                    state = State::OFF;
                }
                break;
        }
    }

    void regulateOutputVoltage() {
        // Smoothly approach the target power
        currentPowerPercent += (targetPowerPercent - currentPowerPercent) * 0.1f;

        float feedbackPercent = getFeedbackVoltagePercent();
        float error = currentPowerPercent - feedbackPercent;

        // Simple proportional controller to adjust PWM output
        outputVoltage += (int)(error * VOLTAGE_KP);
        outputVoltage = constrain(outputVoltage, 0, ANALOG_WRITE_RESOLUTION);

        // Inverted logic: higher PWM value -> faster capacitor discharge -> lower brightness
        analogWrite(VOLTAGE_OUTPUT_PIN, ANALOG_WRITE_RESOLUTION - outputVoltage);
    }
};

#endif // LIGHTING_CONTROLLER_H