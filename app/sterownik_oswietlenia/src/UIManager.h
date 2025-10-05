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
            DEBUG_PRINTF("UI: Button %d, Event %d\n", action.button, action.event);
            display.recordActivity();
            handleInput(action);
        }

        // Refresh screen periodically
        if (millis() - lastUpdate > 500) {
            // Always draw if backlight is on, otherwise only draw if not in edit mode
            if (display.getBacklightState() || editMode == EditMode::NONE) {
                drawCurrentScreen(false); // Don't clear the screen for periodic updates
            }
            lastUpdate = millis();
        }

        // Handle blinking in edit mode
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
                DEBUG_PRINTF("UI: Screen change %d -> %d\n", currentScreen, (currentScreen + 1) % MAIN_MENU_SIZE);
                currentScreen = (currentScreen + 1) % MAIN_MENU_SIZE;
                drawCurrentScreen(true);
                break;
            case BTN_SET:
                if (currentScreen == 1) editMode = EditMode::TIME;
                if (currentScreen == 2) editMode = EditMode::TIMER;
                DEBUG_PRINTF("UI: Entering edit mode %d\n", editMode);
                editPos = 0;
                drawCurrentScreen(true);
                break;
            case BTN_PLUS: // Allow changing timezone without entering edit mode
            case BTN_MINUS:
                if (currentScreen == 3) handleTimezoneEdit(action.button);
                break;
            default:
                break;
        }
    }

    void handleEditInput(ButtonAction action) {
        if (action.button == BTN_SET && action.event == EVENT_PRESS) {
            DEBUG_PRINTLN("UI: Exiting edit mode.");
            editMode = EditMode::NONE;
            settings.save(); // Save settings on exit
            drawCurrentScreen(true); // Clear screen on exiting edit mode
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
                case 0: adjustment = dir * 3600L; break; // Hour
                case 1: adjustment = dir * 60L; break;   // Minute
                case 2: adjustment = dir * 1L; break;    // Second
                case 3: adjustment = dir * 365 * 24 * 3600L; break; // Year
                case 4: adjustment = dir * 30 * 24 * 3600L; break; // Month (approx)
                case 5: adjustment = dir * 24 * 3600L; break; // Day
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
        drawCurrentScreen(false); // Redraw without clearing
    }

    void handleBlinking() {
        bool newBlinkState = (millis() / 400) % 2 == 0;
        if (newBlinkState == blinkState) return; // No change in blink state

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
        if (shouldClear) display.clear();
        switch (currentScreen) {
            case 0: drawInfoScreen(); break; case 1: drawDateScreen(); break; case 2: drawTimerScreen(); break; case 3: drawTimezoneScreen(); break;
        }
    }

    void drawInfoScreen() {
        char buffer[21];
        time_t t = time.toLocal(time.nowUTC(), settings);

        // Line 1: Time (HH:MM), a static label, and a sync indicator '*'
        bool isSyncing = lighting.isSynchronizing();
        snprintf(buffer, sizeof(buffer), "%02d:%02d    LIGHT%c", hour(t), minute(t), isSyncing ? '*' : ' ');
        display.print(0, 0, buffer);

        // Line 2: Power percentage and status (ON/OFF)
        float pwr = lighting.getCurrentPowerPercent();
        bool isOn = pwr > 0.1f; // Use a small threshold to avoid showing ON for 0.0%

        char pwrStr[6];
        dtostrf(pwr, 3, 0, pwrStr);

        snprintf(buffer, sizeof(buffer), "Power: %3s%% %-3s", pwrStr, isOn ? "ON" : "OFF");
        display.print(0, 1, buffer);
    }

    void drawDateScreen() {
        char buffer[21];
        time_t t = time.toLocal(time.nowUTC(), settings);
        
        snprintf(buffer, sizeof(buffer), "Time: %02d:%02d:%02d ", hour(t), minute(t), second(t));
        display.print(0, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Date: %04d-%02d-%02d", year(t), month(t), day(t));
        display.print(0, 1, buffer);

        blinkState = false; // Force redraw of value on screen change
    }

    void drawTimerScreen() {
        char buffer[21];

        snprintf(buffer, sizeof(buffer), "Start: %02d:%02d     ", settings.startHour, settings.startMinute);
        display.print(0, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Stop:  %02d:%02d     ", settings.stopHour, settings.stopMinute);
        display.print(0, 1, buffer);

        blinkState = false; // Force redraw of value on screen change
    }

    void drawTimezoneScreen() {
        display.print(0, 0, "Timezone setting:");

        const char* tzString = (settings.timezone == TZ_WARSAW) ? "Warsaw" : "UTC";

        char buffer[21];
        // Pad with spaces to clear previous longer string (e.g., "Warsaw" -> "UTC")
        snprintf(buffer, sizeof(buffer), " > %-13s", tzString);
        display.print(0, 1, buffer);
    }
};

#endif // UI_MANAGER_H