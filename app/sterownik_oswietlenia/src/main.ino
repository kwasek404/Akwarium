#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>
#include <time.h>
#include <EEPROM.h>
#include <math.h>

const int buttonRightPin = 12;
const int buttonSetPin = 11;
const int buttonMinusPin = 10;
const int buttonPlusPin = 9;
const int screenSelectSize = 3;
const int activityBacklightSeconds = 60;
const int eepromTimerStartHour = 0;
const int eepromTimerStartMinute = 1;
const int eepromTimerStartSecond = 2;
const int eepromTimerStopHour = 3;
const int eepromTimerStopMinute = 4;
const int eepromTimerStopSecond = 5;
const int xPow = 2;
const float yPowMax = 13.4; //6.7 = 103%
const int voltageFeedbackPin = A2;
const int voltagePin = 5;
const int analogReadResolution = 1023;
const int analogWriteResolution = 255;
const int switchTransformerPin = 2;
const int switchBallast1Pin = 3;
const int switchBallast2Pin = 4;
const int switchBallast3Pin = A3;

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
virtuabotixRTC myRTC(6, 7, 8); //If you change the wiring change the pins here also
int screenSelectCurrent = 0;
int buttonRightState = LOW;
int lastButtonRightState = LOW;
int buttonSetState = LOW;
int lastButtonSetState = LOW;
int buttonMinusState = LOW;
int lastButtonMinusState = LOW;
int buttonPlusState = LOW;
int lastButtonPlusState = LOW;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long lastRightDebounceTime = 0;
unsigned long lastSetDebounceTime = 0;
unsigned long lastMinusDebounceTime = 0;
unsigned long lastPlusDebounceTime = 0;

time_t lastActivity = 0;
bool backlightLast = false;
int timerStartHour = 0;
int timerStartMinute = 0;
int timerStartSecond = 0;
int timerStopHour = 0;
int timerStopMinute = 0;
int timerStopSecond = 0;
float powerPercent = 0;
int voltage = 255;
time_t lastTimestamp = 0;
time_t nowTimestamp = 0;
float voltageFeedback;
bool booting = true;
bool prepareForBoot = true;
int switchTransformer = 255;
int switchBallast1 = 255;
int switchBallast2 = 255;
int switchBallast3 = 255;

unsigned long lastLoopTime = 0;
const unsigned long loopInterval = 100; // ms, jak poprzednie delay()

// State for editing screens
bool editingTime = false;
bool editingTimer = false;
int editSelectPos = 0;

// Function prototypes
void screenInfo();
void setSwitches();
void disableSwitches();
void enableSwitches();
void setOutputPower();
void calculatePower();
void eepromLoad();
void eepromSave();
void screenTimer();
void setScreenTimer();
void screenDate();
void adjustTimeDate(struct tm * timeinfo, int hour, int minute, int second, int year, int month, int day);
void setScreenDate();
void setButton();
void screenSelect();
bool getDigitalButton(int pin);
struct tm getCurrentTm();
time_t getCurrentTimestamp();
void setLastActivity();
void setScreenBacklight();



void setup()
{
  Serial.begin(9600);
  lcd.init();
  setLastActivity();
  setScreenBacklight();
  eepromLoad();
  nowTimestamp = getCurrentTimestamp();
  lastTimestamp = nowTimestamp;
}

void loop() {
  // --- Część pętli, która wykonuje się tak szybko, jak to możliwe ---
  // Odczyt przycisków jest najważniejszy, aby nie przegapić wciśnięcia
  setButton();

  // --- Część pętli, która wykonuje się co 'loopInterval' (100ms) ---
  if (millis() - lastLoopTime >= loopInterval) {
    lastLoopTime = millis();

    nowTimestamp = getCurrentTimestamp();
    screenSelect();
    setScreenBacklight();
    setSwitches();
    calculatePower();
    setOutputPower();
    switch (screenSelectCurrent) {
      case 0: // screenInfo
        screenInfo();
        break;
      case 1: // screenDate
        if (editingTime) {
          setScreenDate();
        } else {
          screenDate();
        }
        break;
      case 2: // screenTimer
        if (editingTimer) {
          setScreenTimer();
        } else {
          screenTimer();
        }
        break;
    }
    lastTimestamp = nowTimestamp;

    // Reset button states after they have been processed in the current logic cycle
    buttonRightState = LOW;
    buttonSetState = LOW;
    buttonMinusState = LOW;
    buttonPlusState = LOW;
  }
}

void screenInfo() {
  struct tm timeinfo = getCurrentTm();
  char time[9];
  char date[11];
  char line[2][16];
  char powerPercentStr[8];
  char voltageFeedbackStr[8];
  char enabled[4];
  char b = ' ';

  if (booting) {
    b = 'B';
  }
  
  dtostrf(powerPercent, 4, 1, powerPercentStr);
  dtostrf(11 * voltageFeedback - 10, 4, 1, voltageFeedbackStr);

  strftime(time, 9, "%H:%M:%S", &timeinfo);
  lcd.setCursor(0, 0);
  if (powerPercent > 0) {
    sprintf(line[0], "%s    %c On", time, b);
  } else {
    sprintf(line[0], "%s   %c Off", time, b);
  }
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  sprintf(line[1], "%s%%->%s%%   ", powerPercentStr, voltageFeedbackStr);
  lcd.print(line[1]);
}

void setSwitches() {
  analogWrite(switchTransformerPin, switchTransformer);
  analogWrite(switchBallast1Pin, switchBallast1);
  analogWrite(switchBallast2Pin, switchBallast2);
  analogWrite(switchBallast3Pin, switchBallast3);
}

void disableSwitches() {
  time_t nowTmp = nowTimestamp;
  if (nowTimestamp != lastTimestamp) {
    if (switchBallast1 == 0) {
      switchBallast1 = 255;
      return ;
    }
    if (switchBallast2 == 0) {
      switchBallast2 = 255;
      return ;
    }
    if (switchBallast3 == 0) {
      switchBallast3 = 255;
      return ;
    }
    if (switchTransformer == 0) {
      switchTransformer = 255;
      return ;
    }
  }
}

void enableSwitches() {
  if (switchTransformer == 255) {
    switchTransformer = 0;
    return ;
  }
  if (nowTimestamp != lastTimestamp &&  booting == false) {
    if ((11 * voltageFeedback - 10) >= 1.0) {
      if (switchBallast1 == 255) {
        switchBallast1 = 0;
        return ;
      }
      if (switchBallast2 == 255) {
        switchBallast2 = 0;
        return ;
      }
      if (switchBallast3 == 255) {
        switchBallast3 = 0;
        return ;
      }
    }
  }
}

void setOutputPower() {
  if (lastTimestamp == nowTimestamp) {
    return ;
  }
  voltageFeedback = (analogRead(voltageFeedbackPin)*10)/(float)analogReadResolution;
  if (voltageFeedback >= ((powerPercent+10)/11)) {
    voltage += 1;
  } else {
    voltage -= 1;
  }
  if (voltage < 0) voltage = 0;
  if (voltage > analogWriteResolution) voltage = analogWriteResolution;
  analogWrite(voltagePin, voltage);
  
  if (booting) {
    if ((11 * voltageFeedback - 10) < 0.0) {
      booting = false;
    }
  }
}

void calculatePower() {
  if (booting) {
    enableSwitches();
    powerPercent = 0;
    return ;
  }

  struct tm tmStart = getCurrentTm();
  tmStart.tm_hour = timerStartHour;
  tmStart.tm_min = timerStartMinute;
  tmStart.tm_sec = timerStartSecond;
  struct tm tmStop = getCurrentTm();
  tmStop.tm_hour = timerStopHour;
  tmStop.tm_min = timerStopMinute;
  tmStop.tm_sec = timerStopSecond;

  time_t timestampNow = getCurrentTimestamp();
  time_t timestampStart = mktime(&tmStart);
  time_t timestampStop = mktime(&tmStop);

  if (timestampStart >= timestampStop) {
    tmStop.tm_mday += 1;
    timestampStop = mktime(&tmStop);
  }

  if (! ((timestampStart <= timestampNow) && (timestampStop >= timestampNow))) {
    powerPercent = 0;
    disableSwitches();
    prepareForBoot = true;
    return ;
  }

  if (powerPercent == 0.0 && booting == false && prepareForBoot) {
    booting = true;
    prepareForBoot = false;
    return ;
  }

  enableSwitches();

  time_t timestampstopstartdiff = timestampStop - timestampStart;
  float yPow;

  if ((timestampStart + (timestampstopstartdiff / 2)) >= timestampNow) {
    //rozjasnianie
    yPow = ((yPowMax * (timestampNow - timestampStart)) / (timestampstopstartdiff / 2));
  } else {
    //przyciemnianie
    yPow = ((yPowMax * (timestampStop - timestampNow)) / (timestampstopstartdiff / 2));
    
  }

  powerPercent = pow(xPow, yPow);
  if (powerPercent > 100.0) powerPercent = 100.0;
}

void eepromLoad() {
  timerStartHour = EEPROM.read(eepromTimerStartHour);
  if (timerStartHour >= 24) timerStartHour = 0;
  timerStartMinute = EEPROM.read(eepromTimerStartMinute);
  if (timerStartMinute >= 60) timerStartMinute = 0;
  timerStartSecond = EEPROM.read(eepromTimerStartSecond);
  if (timerStartSecond >= 60) timerStartSecond = 0;
  timerStopHour = EEPROM.read(eepromTimerStopHour);
  if (timerStopHour >= 24) timerStopHour = 0;
  timerStopMinute = EEPROM.read(eepromTimerStopMinute);
  if (timerStopMinute >= 60) timerStopMinute = 0;
  timerStopSecond = EEPROM.read(eepromTimerStopSecond);
  if (timerStopSecond >= 60) timerStopSecond = 0;
}

void eepromSave() {
  EEPROM.write(eepromTimerStartHour, timerStartHour);
  EEPROM.write(eepromTimerStartMinute, timerStartMinute);
  EEPROM.write(eepromTimerStartSecond, timerStartSecond);
  EEPROM.write(eepromTimerStopHour, timerStopHour);
  EEPROM.write(eepromTimerStopMinute, timerStopMinute);
  EEPROM.write(eepromTimerStopSecond, timerStopSecond);
}

void screenTimer() {
  char line[2][16];
  sprintf(line[0], "Start: %.2d:%.2d:%.2d ", timerStartHour, timerStartMinute, timerStartSecond);
  sprintf(line[1], "Stop:  %.2d:%.2d:%.2d ", timerStopHour, timerStopMinute, timerStopSecond);
  lcd.setCursor(0, 0);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  lcd.print(line[1]);
  if (buttonSetState == HIGH) {
    editingTimer = true;
    editSelectPos = 0;
  }
}

void setScreenTimer() {
  char timeStart[9];
  char timeStop[9];
  int menuSize = 6;
  char line[2][16];
  bool blink = (millis() % 1000) < 500; // Blink every 500ms

  if (buttonSetState == HIGH) {
    editingTimer = false;
    eepromSave();
    return;
  }

  sprintf(timeStart, "%.2d:%.2d:%.2d", timerStartHour, timerStartMinute, timerStartSecond);
  sprintf(timeStop, "%.2d:%.2d:%.2d", timerStopHour, timerStopMinute, timerStopSecond);

  if (buttonRightState == HIGH) {
    editSelectPos = (editSelectPos + 1) % menuSize;
  }

  switch (editSelectPos) {
    case 0:
      if (blink) sprintf(timeStart, "  :%.2d:%.2d", timerStartMinute, timerStartSecond);
      if (buttonPlusState == HIGH) timerStartHour = (timerStartHour + 1) % 24;
      if (buttonMinusState == HIGH) timerStartHour = (timerStartHour + 23) % 24;
      break;
    case 1:
      if (blink) sprintf(timeStart, "%.2d:  :%.2d", timerStartHour, timerStartSecond);
      if (buttonPlusState == HIGH) timerStartMinute = (timerStartMinute + 1) % 60;
      if (buttonMinusState == HIGH) timerStartMinute = (timerStartMinute + 59) % 60;
      break;
    case 2:
      if (blink) sprintf(timeStart, "%.2d:%.2d:  ", timerStartHour, timerStartMinute);
      if (buttonPlusState == HIGH) timerStartSecond = (timerStartSecond + 1) % 60;
      if (buttonMinusState == HIGH) timerStartSecond = (timerStartSecond + 59) % 60;
      break;
    case 3:
      if (blink) sprintf(timeStop, "  :%.2d:%.2d", timerStopMinute, timerStopSecond);
      if (buttonPlusState == HIGH) timerStopHour = (timerStopHour + 1) % 24;
      if (buttonMinusState == HIGH) timerStopHour = (timerStopHour + 23) % 24;
      break;
    case 4:
      if (blink) sprintf(timeStop, "%.2d:  :%.2d", timerStopHour, timerStopSecond);
      if (buttonPlusState == HIGH) timerStopMinute = (timerStopMinute + 1) % 60;
      if (buttonMinusState == HIGH) timerStopMinute = (timerStopMinute + 59) % 60;
      break;
    case 5:
      if (blink) sprintf(timeStop, "%.2d:%.2d:  ", timerStopHour, timerStopMinute);
      if (buttonPlusState == HIGH) timerStopSecond = (timerStopSecond + 1) % 60;
      if (buttonMinusState == HIGH) timerStopSecond = (timerStopSecond + 59) % 60;
      break;
  }

  sprintf(line[0], "Start: %s ", timeStart);
  sprintf(line[1], "Stop:  %s ", timeStop);
  lcd.setCursor(0, 0);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  lcd.print(line[1]);
}

void screenDate() {
  struct tm timeinfo = getCurrentTm();
  char time[9];
  char date[11];
  char line[2][16];

  strftime(time, 9, "%H:%M:%S", &timeinfo);
  strftime(date, 11, "%Y/%m/%d", &timeinfo);

  lcd.setCursor(0, 0);
  sprintf(line[0], "Time: %s  ", time);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  sprintf(line[1], "Date: %s", date);
  lcd.print(line[1]);

  if (buttonSetState == HIGH) {
    editingTime = true;
    editSelectPos = 0;
  }
}

void adjustTimeDate(struct tm * timeinfo, int hour, int minute, int second, int year, int month, int day) {
  myRTC.setDS1302Time(timeinfo->tm_sec+second, timeinfo->tm_min+minute, timeinfo->tm_hour+hour, 0, timeinfo->tm_mday+day, timeinfo->tm_mon+month+1, timeinfo->tm_year+year+1900);
}

void setScreenDate() {
  char time[9];
  char date[11];
  char line[2][16];
  char timeFormat[] = "%H:%M:%S";
  char dateFormat[] = "%Y/%m/%d";
  bool blink = (millis() % 1000) < 500; // Blink every 500ms
  int menuSize = 6;
  struct tm timeinfo;

  if (buttonSetState == HIGH) {
    editingTime = false;
    return;
  }

    timeinfo = getCurrentTm();
    strftime(time, 9, timeFormat, &timeinfo);
    strftime(date, 11, dateFormat, &timeinfo);

    if (buttonRightState == HIGH) {
      editSelectPos = (editSelectPos + 1) % menuSize;
    }

    switch (editSelectPos) {
      case 0:
        if (blink) strftime(time, 9, "  :%M:%S", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 1, 0, 0, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, -1, 0, 0, 0, 0, 0);
        break;
      case 1:
        if (blink) strftime(time, 9, "%H:  :%S", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 1, 0, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, -1, 0, 0, 0, 0);
        break;
      case 2:
        if (blink) strftime(time, 9, "%H:%M:  ", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 1, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, -1, 0, 0, 0);
        break;
      case 3:
        if (blink) strftime(date, 11, "    /%m/%d", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 1, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, -1, 0, 0);
        break;
      case 4:
        if (blink) strftime(date, 11, "%Y/  /%d", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, 1, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, -1, 0);
        break;
      case 5:
        if (blink) strftime(date, 11, "%Y/%m/  ", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, 0, 1);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, 0, -1);
        break;
    }

    lcd.setCursor(0, 0);
    sprintf(line[0], "Time: %s ", time);
    lcd.print(line[0]);
    lcd.setCursor(0, 1);
    sprintf(line[1], "Date: %s", date);
    lcd.print(line[1]);
}

void setButton() {
    // Button Right
    int readingRight = getDigitalButton(buttonRightPin);
    if (readingRight != lastButtonRightState) {
        lastRightDebounceTime = millis();
    }
    if ((millis() - lastRightDebounceTime) > debounceDelay) {
        if (readingRight != buttonRightState) {
            if (readingRight == HIGH) {
                buttonRightState = HIGH;
            }
        }
    }
    lastButtonRightState = readingRight;

    // Button Set
    int readingSet = getDigitalButton(buttonSetPin);
    if (readingSet != lastButtonSetState) {
        lastSetDebounceTime = millis();
    }
    if ((millis() - lastSetDebounceTime) > debounceDelay) {
        if (readingSet != buttonSetState) {
            if (readingSet == HIGH) {
                buttonSetState = HIGH;
            }
        }
    }
    lastButtonSetState = readingSet;

    // Button Minus
    int readingMinus = getDigitalButton(buttonMinusPin);
    if (readingMinus != lastButtonMinusState) {
        lastMinusDebounceTime = millis();
    }
    if ((millis() - lastMinusDebounceTime) > debounceDelay) {
        if (readingMinus != buttonMinusState) {
            if (readingMinus == HIGH) {
                buttonMinusState = HIGH;
            }
        }
    }
    lastButtonMinusState = readingMinus;

    // Button Plus
    int readingPlus = getDigitalButton(buttonPlusPin);
    if (readingPlus != lastButtonPlusState) {
        lastPlusDebounceTime = millis();
    }
    if ((millis() - lastPlusDebounceTime) > debounceDelay) {
        if (readingPlus != buttonPlusState) {
            if (readingPlus == HIGH) {
                buttonPlusState = HIGH;
            }
        }
    }
    lastButtonPlusState = readingPlus;
}

void screenSelect() {
  if (editingTime || editingTimer) {
    return; // Don't change screen while editing
  }
  if (buttonRightState == HIGH) {
    screenSelectCurrent = (screenSelectCurrent + 1) % screenSelectSize;
  }
}

bool getDigitalButton(int pin) {
  int state = digitalRead(pin);
  if (state == HIGH) {
    setLastActivity(); // Any press wakes up the backlight timer
    return HIGH;
  }
  return LOW;
}

struct tm getCurrentTm() {
  struct tm timeinfo;
  myRTC.updateTime();
  timeinfo.tm_year = myRTC.year - 1900;
  timeinfo.tm_mon = myRTC.month - 1;
  timeinfo.tm_mday = myRTC.dayofmonth;
  timeinfo.tm_hour = myRTC.hours;
  timeinfo.tm_min = myRTC.minutes;
  timeinfo.tm_sec = myRTC.seconds;
  timeinfo.tm_isdst = -1;
  return timeinfo;
}

time_t getCurrentTimestamp() {
  struct tm timeinfo = getCurrentTm();
  return mktime(&timeinfo);
}

void setLastActivity() {
  lastActivity = getCurrentTimestamp();
}

void setScreenBacklight() {
  bool backlight;
  time_t now = getCurrentTimestamp();
  if (difftime(now, lastActivity) <= activityBacklightSeconds ) {
    backlight = true;
  } else {
    backlight = false;
  }
  if (backlightLast != backlight) {
    backlightLast = backlight;
    if (backlightLast) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }
}
