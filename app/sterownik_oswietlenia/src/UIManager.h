#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "Debug.h"
#include "DisplayController.h"
#include "TimeController.h"
#include "LightingController.h"
#include "InputManager.h"
#include "Settings.h"
#include <TimeLib.h>

class UIManager {
public:
    UIManager(DisplayController& display, TimeController& time, LightingController& lighting, InputProcessor& input, Settings& settings)
        : display(display), time(time), lighting(lighting), inputProcessor(input), settings(settings), currentScreen(0), lastUpdate(0) {}

    void update() {
        ButtonAction action = inputProcessor.getAction();
        if (action.button != BTN_NONE) {
            bool backlightWasOn = display.getBacklightState();
            display.recordActivity();

            // First button press on a dark screen should only wake it up.
            if (backlightWasOn) {
                handleInput(action);
            }
        }

        // Refresh screen periodically
        if (millis() - lastUpdate > 500) {
            if (display.getBacklightState()) {
                drawCurrentScreen(false);
            }
            lastUpdate = millis();
        }
    }

private:
    enum class EditMode { NONE, TIME, TIMER };
    
    DisplayController& display;
    TimeController& time;
    LightingController& lighting;
    InputProcessor& inputProcessor;
    Settings& settings;

    int currentScreen;
    EditMode editMode = EditMode::NONE;
    int editPos = 0;
    unsigned long lastUpdate;

    void handleInput(ButtonAction action) {
        if (editMode != EditMode::NONE) {
            handleEditInput(action);
            drawCurrentScreen(false);
            return;
        }

        if (action.event != EVENT_PRESS) return;

        switch (action.button) {
            case BTN_RIGHT:
                currentScreen = (currentScreen + 1) % MAIN_MENU_SIZE;
                drawCurrentScreen(true);
                break;
            case BTN_SET:
                if (currentScreen == 1) {
                    editMode = EditMode::TIME;
                    editPos = 0;
                    drawCurrentScreen(true);
                }
                if (currentScreen == 2) {
                    editMode = EditMode::TIMER;
                    editPos = 0;
                    drawCurrentScreen(true);
                }
                break;
            case BTN_PLUS:
            case BTN_MINUS:
                if (currentScreen == 3) handleTimezoneEdit(action.button);
                break;
            default:
                break;
        }
    }

    void handleEditInput(ButtonAction action) {
        if (action.button == BTN_SET && action.event == EVENT_PRESS) {
            editMode = EditMode::NONE;
            settings.save();
            drawCurrentScreen(true);
            return;
        }

        if (editMode == EditMode::TIME) {
            handleTimeEdit(action);
        } else if (editMode == EditMode::TIMER) {
            handleTimerEdit(action);
        }
    }

    void handleTimeEdit(ButtonAction action) {
        long adjustment = 0;
        if (action.button == BTN_RIGHT && action.event == EVENT_PRESS) {
            editPos = (editPos + 1) % 6;
        }
        
        if (action.button == BTN_PLUS || action.button == BTN_MINUS) {
            int dir = (action.button == BTN_PLUS) ? 1 : -1;
            switch (editPos) {
                case 0: adjustment = dir * 3600L; break;
                case 1: adjustment = dir * 60L; break;
                case 2: adjustment = dir * 1L; break;
                case 3: adjustment = dir * 365 * 24 * 3600L; break;
                case 4: adjustment = dir * 30 * 24 * 3600L; break;
                case 5: adjustment = dir * 24 * 3600L; break;
            }
            time.adjustTime(adjustment, settings);
        }
    }

    void handleTimerEdit(ButtonAction action) {
        if (action.button == BTN_RIGHT && action.event == EVENT_PRESS) {
            editPos = (editPos + 1) % 4;
        }

        if (action.button == BTN_PLUS || action.button == BTN_MINUS) {
            int dir = (action.button == BTN_PLUS) ? 1 : -1;
            switch (editPos) {
                case 0: settings.startHour = (settings.startHour + dir + 24) % 24; break;
                case 1: settings.startMinute = (settings.startMinute + dir + 60) % 60; break;
                case 2: settings.stopHour = (settings.stopHour + dir + 24) % 24; break;
                case 3: settings.stopMinute = (settings.stopMinute + dir + 60) % 60; break;
            }
        }
    }

    void handleTimezoneEdit(Button btn) {
        if (settings.timezone == TZ_UTC) {
            settings.timezone = TZ_WARSAW;
        } else {
            settings.timezone = TZ_UTC;
        }
        settings.save();
        drawCurrentScreen(false);
    }

    void drawCurrentScreen(bool shouldClear) {
        if (shouldClear) display.clear();
        switch (currentScreen) {
            case 0: drawInfoScreen(); break;
            case 1: drawDateScreen(); break;
            case 2: drawTimerScreen(); break;
            case 3: drawTimezoneScreen(); break;
        }
    }

    void drawInfoScreen() {
        char buffer[17];
        time_t t = time.toLocal(time.nowUTC(), settings);

        snprintf(buffer, sizeof(buffer), "%02d:%02d %-9s", hour(t), minute(t), lighting.getCurrentPhaseName());
        display.print(0, 0, buffer);

        float pwr = lighting.getCurrentPowerPercent();
        char pwrStr[6];
        dtostrf(pwr, 3, 0, pwrStr);

        uint8_t mask = lighting.getActiveBallastMask();
        char t1 = (mask & BALLAST_1) ? '1' : '-';
        char t2 = (mask & BALLAST_1) ? '2' : '-';
        char t3 = (mask & BALLAST_2) ? '3' : '-';
        char t4 = (mask & BALLAST_2) ? '4' : '-';
        char t5 = (mask & BALLAST_3) ? '5' : '-';

        snprintf(buffer, sizeof(buffer), "P:%3s%% B:%c%c%c%c%c", pwrStr, t1, t2, t3, t4, t5);
        display.print(0, 1, buffer);
    }

    void drawDateScreen() {
        char buffer[17];
        time_t t = time.toLocal(time.nowUTC(), settings);
        
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour(t), minute(t), second(t));
        display.print(4, 0, buffer);
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year(t), month(t), day(t));
        display.print(3, 1, buffer);
    }

    void drawTimerScreen() {
        char buffer[17];
        snprintf(buffer, sizeof(buffer), "Start: %02d:%02d", settings.startHour, settings.startMinute);
        display.print(0, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Stop:  %02d:%02d", settings.stopHour, settings.stopMinute);
        display.print(0, 1, buffer);
    }

    void drawTimezoneScreen() {
        display.print(0, 0, "Timezone:");
        const char* tzString = (settings.timezone == TZ_WARSAW) ? "Warsaw" : "UTC";
        char buffer[17];
        snprintf(buffer, sizeof(buffer), "> %s", tzString);
        display.print(0, 1, buffer);
    }
};

#endif // UI_MANAGER_H