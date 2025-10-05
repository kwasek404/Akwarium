#include <Arduino.h>

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
    Serial.begin(9600);
    DEBUG_PRINTLN("\n\n--- Aquarium Controller Starting ---");

    // Initialize hardware and load settings
    DEBUG_PRINTLN("Initializing Display...");
    displayController.begin();
    DEBUG_PRINTLN("Initializing Time...");
    timeController.begin();
    DEBUG_PRINTLN("Loading Settings...");
    settings.load();
    
    displayController.print(0, 0, "Aquarium Controller");
    displayController.print(0, 1, "Starting...");
    delay(2000);
    displayController.clear();
}

void loop() {
    // Get current time once per loop
    time_t utc_now = timeController.nowUTC();
    time_t local_now = timeController.toLocal(utc_now, settings);
    
    // Update all controllers
    lightingController.update(local_now, settings);
    displayController.update();
    uiManager.update();
}