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
    UIManager(DisplayController& display, TimeController& time, LightingController& lighting, InputManager& input, Settings& settings)
        : display(display), time(time), lighting(lighting), input(input), settings(settings), currentScreen(0), lastUpdate(0) {}

    void update() {
        Button btn = input.getPressedButton();
        if (btn != BTN_NONE) {
            display.recordActivity();
            handleInput(btn);
        }

        // Refresh screen periodically
        if (millis() - lastUpdate > 500) {
            // Always draw if backlight is on, otherwise only draw if not in edit mode
            if (display.getBacklightState() || editMode == EditMode::NONE) {
                drawCurrentScreen(false); // Don't clear the screen for periodic updates
            }
            lastUpdate = millis();
        }
    }

private:
    enum class EditMode { NONE, TIME, TIMER };
    
    DisplayController& display;
    TimeController& time;
    LightingController& lighting;
    InputManager& input;
    Settings& settings;

    int currentScreen;
    EditMode editMode = EditMode::NONE;
    int editPos = 0;
    unsigned long lastUpdate;

    void handleInput(Button btn) {
        if (editMode != EditMode::NONE) {
            handleEditInput(btn);
            drawCurrentScreen(false); // Redraw without clearing
            return;
        }

        switch (btn) {
            case BTN_RIGHT:
                currentScreen = (currentScreen + 1) % MAIN_MENU_SIZE;
                drawCurrentScreen(true); // Clear screen on view change
                break;
            case BTN_SET:
                if (currentScreen == 1) editMode = EditMode::TIME;
                if (currentScreen == 2) editMode = EditMode::TIMER;
                editPos = 0;
                drawCurrentScreen(true); // Clear screen when entering edit mode
                break;
            case BTN_PLUS: // Allow changing timezone without entering edit mode
            case BTN_MINUS:
                if (currentScreen == 3) handleTimezoneEdit();
                break;
            default:
                break;
        }
    }

    void handleEditInput(Button btn) {
        if (btn == BTN_SET) {
            editMode = EditMode::NONE;
            settings.save(); // Save settings on exit
            drawCurrentScreen(true); // Clear screen on exiting edit mode
            return;
        }

        if (editMode == EditMode::TIME) {
            handleTimeEdit(btn);
        } else if (editMode == EditMode::TIMER) {
            handleTimerEdit(btn);
        }
    }

    void handleTimeEdit(Button btn) {
        long adjustment = 0;
        if (btn == BTN_RIGHT) editPos = (editPos + 1) % 6;
        
        if (btn == BTN_PLUS || btn == BTN_MINUS) {
            int dir = (btn == BTN_PLUS) ? 1 : -1;
            switch (editPos) {
                case 0: adjustment = dir * 3600L; break; // Hour
                case 1: adjustment = dir * 60L; break;   // Minute
                case 2: adjustment = dir * 1L; break;    // Second
                case 3: time.adjustTime(dir * 365 * 24 * 3600L); return; // Year
                case 4: time.adjustTime(dir * 30 * 24 * 3600L); return; // Month (approx)
                case 5: time.adjustTime(dir * 24 * 3600L); return; // Day
            }
            time.adjustTime(adjustment);
        }
    }

    void handleTimerEdit(Button btn) {
        if (btn == BTN_RIGHT) editPos = (editPos + 1) % 4;

        if (btn == BTN_PLUS || btn == BTN_MINUS) {
            int dir = (btn == BTN_PLUS) ? 1 : -1;
            switch (editPos) {
                case 0: settings.startHour = (settings.startHour + dir + 24) % 24; break;
                case 1: settings.startMinute = (settings.startMinute + dir + 60) % 60; break;
                case 2: settings.stopHour = (settings.stopHour + dir + 24) % 24; break;
                case 3: settings.stopMinute = (settings.stopMinute + dir + 60) % 60; break;
            }
        }
    }

    void handleTimezoneEdit() {
        if (settings.timezone == TZ_UTC) {
            settings.timezone = TZ_WARSAW;
        } else {
            settings.timezone = TZ_UTC;
        }
        settings.save();
        drawCurrentScreen(false); // Redraw without clearing
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
        
        // Always draw the full text first
        snprintf(buffer, sizeof(buffer), "Time: %02d:%02d:%02d ", hour(t), minute(t), second(t));
        display.print(0, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Date: %04d-%02d-%02d", year(t), month(t), day(t));
        display.print(0, 1, buffer);

        // Then, if in edit mode, overwrite the blinking part with spaces
        if (editMode == EditMode::TIME && (millis() / 400) % 2 == 0) {
            switch (editPos) {
                case 0: display.print(6, 0, "  "); break;  // Hour
                case 1: display.print(9, 0, "  "); break;  // Minute
                case 2: display.print(12, 0, "  "); break; // Second
                case 3: display.print(6, 1, "    "); break; // Year
                case 4: display.print(11, 1, "  "); break; // Month
                case 5: display.print(14, 1, "  "); break; // Day
            }
        }
    }

    void drawTimerScreen() {
        char buffer[21];

        // Always draw the full text first
        snprintf(buffer, sizeof(buffer), "Start: %02d:%02d     ", settings.startHour, settings.startMinute);
        display.print(0, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Stop:  %02d:%02d     ", settings.stopHour, settings.stopMinute);
        display.print(0, 1, buffer);

        // Then, if in edit mode, overwrite the blinking part with spaces
        if (editMode == EditMode::TIMER && (millis() / 400) % 2 == 0) {
            switch (editPos) {
                case 0: display.print(7, 0, "  "); break; // Start Hour
                case 1: display.print(10, 0, "  "); break; // Start Minute
                case 2: display.print(7, 1, "  "); break; // Stop Hour
                case 3: display.print(10, 1, "  "); break; // Stop Minute
            }
        }
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