#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "Constants.h"

enum Button {
    BTN_NONE, BTN_RIGHT, BTN_SET, BTN_MINUS, BTN_PLUS
};

enum ButtonEvent {
    EVENT_NONE,
    EVENT_PRESS, // Short press
    EVENT_HOLD   // Long press / repeat
};

class InputManager {
public:
    InputManager() {
        pinMode(BUTTON_RIGHT_PIN, INPUT);
        pinMode(BUTTON_SET_PIN, INPUT);
        pinMode(BUTTON_MINUS_PIN, INPUT);
        pinMode(BUTTON_PLUS_PIN, INPUT);
    }

    ButtonEvent checkButton(Button btn, ButtonEvent& event) {
        event = EVENT_NONE;
        uint8_t index = btn - 1;
        bool currentState = (digitalRead(buttonPins[index]) == HIGH);

        if (currentState != buttonStates[index].lastReading) {
            buttonStates[index].lastDebounceTime = millis();
        }
        buttonStates[index].lastReading = currentState;

        if ((millis() - buttonStates[index].lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
            if (currentState && !buttonStates[index].isPressed) { // Button just pressed
                buttonStates[index].isPressed = true;
                buttonStates[index].pressTime = millis();
                event = EVENT_PRESS;
            } else if (currentState && buttonStates[index].isPressed) { // Button is being held
                if ((millis() - buttonStates[index].pressTime) > LONG_PRESS_DELAY) {
                    if ((millis() - buttonStates[index].lastRepeatTime) > HOLD_REPEAT_DELAY) {
                        buttonStates[index].lastRepeatTime = millis();
                        event = EVENT_HOLD;
                    }
                }
            } else if (!currentState && buttonStates[index].isPressed) { // Button released
                buttonStates[index].isPressed = false;
            }
        }

        return event;
    }

private:
    struct ButtonState {
        bool isPressed = false;
        bool lastReading = false;
        unsigned long pressTime = 0;
        unsigned long lastDebounceTime = 0;
        unsigned long lastRepeatTime = 0;
    };

    const uint8_t buttonPins[4] = {BUTTON_RIGHT_PIN, BUTTON_SET_PIN, BUTTON_MINUS_PIN, BUTTON_PLUS_PIN};
    ButtonState buttonStates[4];
};

struct ButtonAction {
    Button button;
    ButtonEvent event;
};

class InputProcessor {
public:
    InputProcessor(InputManager& manager) : inputManager(manager) {}

    ButtonAction getAction() {
        ButtonEvent event;
        for (int i = 0; i < 4; i++) {
            Button btn = (Button)(i + 1);
            if (inputManager.checkButton(btn, event) != EVENT_NONE) {
                return {btn, event};
            }
        }
        return {BTN_NONE, EVENT_NONE};
    }
private:
    InputManager& inputManager;
};

#endif // INPUT_MANAGER_H