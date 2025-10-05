#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "Constants.h"

enum Button {
    BTN_NONE,
    BTN_RIGHT,
    BTN_SET,
    BTN_MINUS,
    BTN_PLUS
};

class InputManager {
public:
    InputManager() {
        pinMode(BUTTON_RIGHT_PIN, INPUT);
        pinMode(BUTTON_SET_PIN, INPUT);
        pinMode(BUTTON_MINUS_PIN, INPUT);
        pinMode(BUTTON_PLUS_PIN, INPUT);
    }

    Button getPressedButton() {
        // Check each button independently. This prevents one button press from masking another.
        if (isButtonPressed(BUTTON_RIGHT_PIN, lastRightState, lastReadingRight, lastRightDebounceTime)) {
            return BTN_RIGHT;
        }
        if (isButtonPressed(BUTTON_SET_PIN, lastSetState, lastReadingSet, lastSetDebounceTime)) {
            return BTN_SET;
        }
        if (isButtonPressed(BUTTON_MINUS_PIN, lastMinusState, lastReadingMinus, lastMinusDebounceTime)) {
            return BTN_MINUS;
        }
        if (isButtonPressed(BUTTON_PLUS_PIN, lastPlusState, lastReadingPlus, lastPlusDebounceTime)) {
            return BTN_PLUS;
        }

        return BTN_NONE;
    }

private:
    int lastRightState = LOW;
    int lastSetState = LOW;
    int lastMinusState = LOW;
    int lastPlusState = LOW;

    unsigned long lastRightDebounceTime = 0;
    unsigned long lastSetDebounceTime = 0;
    unsigned long lastMinusDebounceTime = 0;
    unsigned long lastPlusDebounceTime = 0;
    
    // Add last reading state to correctly detect changes
    int lastReadingRight = LOW;
    int lastReadingSet = LOW;
    int lastReadingMinus = LOW;
    int lastReadingPlus = LOW;

    bool isButtonPressed(uint8_t pin, int& lastState, int& lastReading, unsigned long& lastDebounceTime) {
        int reading = digitalRead(pin);

        // Reset the debounce timer whenever the input changes
        if (reading != lastReading) {
            lastDebounceTime = millis();
        }
        lastReading = reading;

        if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
            // If the reading has been stable for the debounce period,
            // and it's different from the last registered state, update the state.
            if (reading != lastState) {
                lastState = reading;
                // A press is registered only on the transition from LOW to HIGH
                if (lastState == HIGH) {
                    return true;
                }
            }
        }
        return false;
    }
};

#endif // INPUT_MANAGER_H