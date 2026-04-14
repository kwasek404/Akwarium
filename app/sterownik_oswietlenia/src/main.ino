#include <Arduino.h>
#include <avr/wdt.h>

// Include all controller and settings headers
#include "Constants.h"
#include "Debug.h"
#include "Settings.h"
#include "TimeController.h"
#include "DisplayController.h"
#include "InputManager.h"
#include "LightingController.h"
#include "UIManager.h"

// Global objects for our controllers
Settings settings;
TimeController timeController(RTC_CLK_PIN, RTC_DAT_PIN, RTC_RST_PIN);
DisplayController displayController(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
InputManager inputManager;
InputProcessor inputProcessor(inputManager);
LightingController lightingController;
UIManager uiManager(displayController, timeController, lightingController, inputProcessor, settings);

void setup() {
    wdt_disable();

    pinMode(SWITCH_TRANSFORMER_PIN, OUTPUT);
    digitalWrite(SWITCH_TRANSFORMER_PIN, HIGH);

    pinMode(RTC_RST_PIN, OUTPUT);
    digitalWrite(RTC_RST_PIN, LOW);

    displayController.begin();

    delay(500);

    timeController.begin();
    settings.load();

    delay(500);
    lightingController.begin();
    displayController.clear();

    wdt_enable(WDTO_2S);
}

void loop() {
    wdt_reset();

    time_t utc_now = timeController.nowUTC();
    time_t local_now = timeController.toLocal(utc_now, settings);
    
    // Update all controllers
    lightingController.update(local_now, settings);

    if (lightingController.relaySwitched) {
        lightingController.relaySwitched = false;
        timeController.suppressReads(500);
        delay(100);
        displayController.reinit();
    }

    displayController.update();
    uiManager.update();
}