#ifndef UI_MANAGER_H
#define UI_MANAGER_H

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

    

            // Refresh screen periodically, but handle blinking separately

            if (editMode == EditMode::NONE && millis() - lastUpdate > 500) {

                if (display.getBacklightState()) {

                    drawCurrentScreen(false);

                }

                lastUpdate = millis();

            }

            

            if (editMode != EditMode::NONE) {

                handleBlinking();

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

        bool blinkState = false;

    

        void handleInput(ButtonAction action) {

            if (editMode != EditMode::NONE) {

                handleEditInput(action);

                drawCurrentScreen(false);

                return;

            }

    

            // Only handle short presses for navigation

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

    

        void handleBlinking() {

            bool newBlinkState = (millis() / 400) % 2 == 0;

            if (newBlinkState == blinkState) return;

            blinkState = newBlinkState;

    

            time_t t = time.toLocal(time.nowUTC(), settings);

            char buffer[5];

            char space_buffer[5];

    

            if (editMode == EditMode::TIME) {

                const uint8_t positions[6][2] = {{6, 0}, {9, 0}, {12, 0}, {6, 1}, {11, 1}, {14, 1}};

                const uint8_t lengths[6] = {2, 2, 2, 4, 2, 2};

                const int values[6] = {hour(t), minute(t), second(t), year(t), month(t), day(t)};

                const char* formats[6] = {"%02d", "%02d", "%02d", "%04d", "%02d", "%02d"};

    

                if (blinkState) {

                    memset(space_buffer, ' ', lengths[editPos]);

                    space_buffer[lengths[editPos]] = '\0';

                    display.print(positions[editPos][0], positions[editPos][1], space_buffer);

                } else {

                    snprintf(buffer, sizeof(buffer), formats[editPos], values[editPos]);

                    display.print(positions[editPos][0], positions[editPos][1], buffer);

                }

            } else if (editMode == EditMode::TIMER) {

                const uint8_t positions[4][2] = {{7, 0}, {10, 0}, {7, 1}, {10, 1}};

                const int values[4] = {settings.startHour, settings.startMinute, settings.stopHour, settings.stopMinute};

    

                if (blinkState) {

                    memset(space_buffer, ' ', 2);

                    space_buffer[2] = '\0';

                    display.print(positions[editPos][0], positions[editPos][1], space_buffer);

                } else {

                    snprintf(buffer, sizeof(buffer), "%02d", values[editPos]);

                    display.print(positions[editPos][0], positions[editPos][1], buffer);

                }

            }

        }

    void drawCurrentScreen(bool shouldClear) {
        if (shouldClear) {
             display.clear();
        }
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

        // --- Line 1: Time and Power ---
        float perBallastPwr = lighting.getCurrentPowerPercent();
        float globalPwr = lighting.getGlobalPowerPercent();
        char p1[4], p2[4];
        dtostrf(perBallastPwr, 3, 0, p1);
        dtostrf(globalPwr, 3, 0, p2);
        // HH:MM P:XXX/YYY% -> 16 chars max
        snprintf(buffer, sizeof(buffer), "%02d:%02d P:%3s/%3s%%", hour(t), minute(t), p1, p2);
        display.print(0, 0, buffer);

        // --- Line 2: Phase Name and Ballasts ---
        uint8_t mask = lighting.getActiveBallastMask();
        char t1 = (mask & BALLAST_1) ? '1' : '-';
        char t2 = (mask & BALLAST_1) ? '2' : '-';
        char t3 = (mask & BALLAST_2) ? '3' : '-';
        char t4 = (mask & BALLAST_2) ? '4' : '-';
        char t5 = (mask & BALLAST_3) ? '5' : '-';
        // PhaseName B:12345 -> max 10 for name + " 12345" = 16 chars
        snprintf(buffer, sizeof(buffer), "%-10s %c%c%c%c%c", lighting.getCurrentPhaseName(), t1, t2, t3, t4, t5);
        display.print(0, 1, buffer);
    }

    void drawDateScreen() {
        char buffer[17];
        time_t t = time.toLocal(time.nowUTC(), settings);
        
        snprintf(buffer, sizeof(buffer), "Time: %02d:%02d:%02d", hour(t), minute(t), second(t));
        display.print(0, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Date: %04d-%02d-%02d", year(t), month(t), day(t));
        display.print(0, 1, buffer);
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
        snprintf(buffer, sizeof(buffer), "> %-13s", tzString);
        display.print(0, 1, buffer);
    }
};

#endif // UI_MANAGER_H