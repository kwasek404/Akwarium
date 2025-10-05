#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <LiquidCrystal_I2C.h>
#include "Constants.h"

class DisplayController {
public:
    DisplayController(uint8_t addr, uint8_t cols, uint8_t rows) : lcd(addr, cols, rows) {}

    void begin() {
        lcd.init();
        lcd.backlight();
        lastActivityTime = millis();
        isBacklightOn = true;
    }

    void update() {
        if (isBacklightOn && (millis() - lastActivityTime > ACTIVITY_BACKLIGHT_SECONDS * 1000)) {
            lcd.noBacklight();
            isBacklightOn = false;
        }
    }

    void recordActivity() {
        if (!isBacklightOn) {
            lcd.backlight();
            isBacklightOn = true;
        }
        lastActivityTime = millis();
    }

    void print(uint8_t col, uint8_t row, const char* text) {
        lcd.setCursor(col, row);
        lcd.print(text);
    }

    void clear() {
        lcd.clear();
    }

    bool getBacklightState() {
        return isBacklightOn;
    }

private:
    LiquidCrystal_I2C lcd;
    unsigned long lastActivityTime;
    bool isBacklightOn;
};

#endif // DISPLAY_CONTROLLER_H