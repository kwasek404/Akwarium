#ifndef LIGHTING_CONTROLLER_H
#define LIGHTING_CONTROLLER_H

#include "Constants.h"
#include "Settings.h"
#include <TimeLib.h>

class LightingController {
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
        // Formula from original code: 11 * voltageFeedback - 10
        // voltageFeedback = (analogRead * 10) / 1023
        // So, (11 * (analogRead * 10) / 1023) - 10
        float feedback = (analogRead(VOLTAGE_FEEDBACK_PIN) * 10.0f) / ANALOG_READ_RESOLUTION;
        return 11.0f * feedback - 10.0f;
    }

    bool isSynchronizing() const {
        // Consider it synchronizing if the difference between target and actual power is significant
        float error = targetPowerPercent - getFeedbackVoltagePercent();
        return abs(error) > 5.0f; // 5% threshold
    }

private:
    enum class State { OFF, RAMPING_UP, ON, RAMPING_DOWN, BOOTING };
    State state = State::OFF;
    
    float targetPowerPercent = 0.0f;
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
        if (targetPowerPercent > 100.0f) targetPowerPercent = 100.0f;
        if (targetPowerPercent < 0.0f) targetPowerPercent = 0.0f;
    }

    void manageSwitches() {
        bool shouldBeOn = targetPowerPercent > 0.0f;
        unsigned long currentTime = millis();

        if (shouldBeOn) {
            // Turn on transformer first
            digitalWrite(SWITCH_TRANSFORMER_PIN, LOW);

            // Sequentially turn on ballasts after a delay
            if (currentTime - lastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS) {
                if (ballastsOn == 0) {
                    digitalWrite(SWITCH_BALLAST_1_PIN, LOW);
                    ballastsOn = 1;
                    lastSwitchTime = currentTime;
                } else if (ballastsOn == 1) {
                    digitalWrite(SWITCH_BALLAST_2_PIN, LOW);
                    ballastsOn = 2;
                    lastSwitchTime = currentTime;
                } else if (ballastsOn == 2) {
                    digitalWrite(SWITCH_BALLAST_3_PIN, LOW);
                    ballastsOn = 3;
                }
            }
        } else {
            // Turn off in reverse order
            if (currentTime - lastSwitchTime > SEQUENTIAL_SWITCH_DELAY_MS) {
                 if (ballastsOn == 3) {
                    digitalWrite(SWITCH_BALLAST_3_PIN, HIGH);
                    ballastsOn = 2;
                    lastSwitchTime = currentTime;
                } else if (ballastsOn == 2) {
                    digitalWrite(SWITCH_BALLAST_2_PIN, HIGH);
                    ballastsOn = 1;
                    lastSwitchTime = currentTime;
                } else if (ballastsOn == 1) {
                    digitalWrite(SWITCH_BALLAST_1_PIN, HIGH);
                    ballastsOn = 0;
                    lastSwitchTime = currentTime;
                } else if (ballastsOn == 0) {
                    digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);
                }
            }
        }
    }

    void regulateOutputVoltage() {
        currentPowerPercent = targetPowerPercent; // For simplicity, we can add smoothing later
        
        float feedbackPercent = getFeedbackVoltagePercent();
        float error = currentPowerPercent - feedbackPercent;

        // Simple proportional controller to adjust voltage
        outputVoltage += (int)(error * VOLTAGE_KP);

        outputVoltage = constrain(outputVoltage, 0, ANALOG_WRITE_RESOLUTION);
        analogWrite(VOLTAGE_OUTPUT_PIN, outputVoltage);
    }
};

#endif // LIGHTING_CONTROLLER_H