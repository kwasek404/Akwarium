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

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
virtuabotixRTC myRTC(6, 7, 8); //If you change the wiring change the pins here also
int screenSelectCurrent = 0;
int buttonRightState = LOW;
int buttonSetState = LOW;
int buttonMinusState = LOW;
int buttonPlusState = LOW;
time_t lastActivity = 0;
bool backlightLast = false;
int timerStartHour = 0;
int timerStartMinute = 0;
int timerStartSecond = 0;
int timerStopHour = 0;
int timerStopMinute = 0;
int timerStopSecond = 0;
float powerPercent = 0;

void setup()
{
  Serial.begin(9600);
  lcd.init();
  setLastActivity();
  setScreenBacklight();
  eepromLoad();
}

void loop() {
  setButton();
  screenSelect();
  setScreenBacklight();
  calculatePower();
  switch (screenSelectCurrent) {
    case 0:
      screenInfo();
      break;
    case 1:
      screenDate();
      break;
    case 2:
      screenTimer();
      break;
  }
  delay(100);
}

void screenInfo() {
  struct tm timeinfo = getCurrentTm();
  char time[9];
  char date[11];
  char line[2][16];
  strftime(time, 9, "%H:%M:%S", &timeinfo);
  lcd.setCursor(0, 0);
  sprintf(line[0], "Time: %s ", time);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  sprintf(line[1], "Power: %s%%    ", String(powerPercent).c_str());
  lcd.print(line[1]);
}

void calculatePower() {
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
    return ;
  }

  time_t timestampstopstartdiff = timestampStop - timestampStart;
  float yPow;

  if ((timestampStart + (timestampstopstartdiff / 2)) >= timestampNow) {
    //rozjasnianie
    Serial.println("rozjasnianie");
    yPow = ((yPowMax * (timestampNow - timestampStart)) / (timestampstopstartdiff / 2));
  } else {
    //przyciemnianie
    Serial.println("przyciemnianie");
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
  if (buttonSetState == HIGH) setScreenTimer();
}

void setScreenTimer() {
  char timeStart[9];
  char timeStop[9];
  int selectPos = 0;
  int menuSize = 6;
  char line[2][16];
  bool display = true;

  do {
    setButton();
    sprintf(timeStart, "%.2d:%.2d:%.2d", timerStartHour, timerStartMinute, timerStartSecond);
    sprintf(timeStop, "%.2d:%.2d:%.2d", timerStopHour, timerStopMinute, timerStopSecond);

    if (buttonRightState == HIGH) {
      selectPos += 1;
      if (selectPos >= menuSize) {
        selectPos = 0;
      }
    }

    switch (selectPos) {
      case 0:
        if (!display) sprintf(timeStart, "  :%.2d:%.2d", timerStartMinute, timerStartSecond);
        if (buttonPlusState == HIGH) timerStartHour += 1;
        if (buttonMinusState == HIGH) timerStartHour += -1;
        break;
      case 1:
        if (!display) sprintf(timeStart, "%.2d:  :%.2d", timerStartHour, timerStartSecond);
        if (buttonPlusState == HIGH) timerStartMinute += 1;
        if (buttonMinusState == HIGH) timerStartMinute += -1;
        break;
      case 2:
        if (!display) sprintf(timeStart, "%.2d:%.2d:  ", timerStartHour, timerStartMinute);
        if (buttonPlusState == HIGH) timerStartSecond += 1;
        if (buttonMinusState == HIGH) timerStartSecond += -1;
        break;
      case 3:
        if (!display) sprintf(timeStop, "  :%.2d:%.2d", timerStopMinute, timerStopSecond);
        if (buttonPlusState == HIGH) timerStopHour += 1;
        if (buttonMinusState == HIGH) timerStopHour += -1;
        break;
      case 4:
        if (!display) sprintf(timeStop, "%.2d:  :%.2d", timerStopHour, timerStopSecond);
        if (buttonPlusState == HIGH) timerStopMinute += 1;
        if (buttonMinusState == HIGH) timerStopMinute += -1;
        break;
      case 5:
        if (!display) sprintf(timeStop, "%.2d:%.2d:  ", timerStopHour, timerStopMinute);
        if (buttonPlusState == HIGH) timerStopSecond += 1;
        if (buttonMinusState == HIGH) timerStopSecond += -1;
        break;
    }

    if (display) {
      display = false;
    } else {
      display = true;
    }

    sprintf(line[0], "Start: %s ", timeStart);
    sprintf(line[1], "Stop:  %s ", timeStop);
    lcd.setCursor(0, 0);
    lcd.print(line[0]);
    lcd.setCursor(0, 1);
    lcd.print(line[1]);

    delay(100);
  } while (buttonSetState == LOW);
  eepromSave();
}

void screenDate() {
  struct tm timeinfo = getCurrentTm();
  char time[9];
  char date[11];
  char line[2][16];

  strftime(time, 9, "%H:%M:%S", &timeinfo);
  strftime(date, 11, "%Y/%m/%d", &timeinfo);

  lcd.setCursor(0, 0);
  sprintf(line[0], "Time: %s ", time);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  sprintf(line[1], "Date: %s", date);
  lcd.print(line[1]);

  if (buttonSetState == HIGH) setScreenDate();
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
  bool display = true;
  int selectPos = 0;
  int menuSize = 6;
  struct tm timeinfo;
  do {
    setButton();
    timeinfo = getCurrentTm();
    strftime(time, 9, timeFormat, &timeinfo);
    strftime(date, 11, dateFormat, &timeinfo);

    if (buttonRightState == HIGH) {
      selectPos += 1;
      if (selectPos >= menuSize) {
        selectPos = 0;
      }
    }

    switch (selectPos) {
      case 0:
        if (!display) strftime(time, 9, "  :%M:%S", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 1, 0, 0, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, -1, 0, 0, 0, 0, 0);
        break;
      case 1:
        if (!display) strftime(time, 9, "%H:  :%S", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 1, 0, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, -1, 0, 0, 0, 0);
        break;
      case 2:
        if (!display) strftime(time, 9, "%H:%M:  ", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 1, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, -1, 0, 0, 0);
        break;
      case 3:
        if (!display) strftime(date, 11, "    /%m/%d", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 1, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, -1, 0, 0);
        break;
      case 4:
        if (!display) strftime(date, 11, "%Y/  /%d", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, 1, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, -1, 0);
        break;
      case 5:
        if (!display) strftime(date, 11, "%Y/%m/  ", &timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, 0, 1);
        if (buttonMinusState == HIGH) adjustTimeDate(&timeinfo, 0, 0, 0, 0, 0, -1);
        break;
    }

    if (display) {
      display = false;
    } else {
      display = true;
    }

    lcd.setCursor(0, 0);
    sprintf(line[0], "Time: %s ", time);
    lcd.print(line[0]);
    lcd.setCursor(0, 1);
    sprintf(line[1], "Date: %s", date);
    lcd.print(line[1]);
    delay(100);
  } while (buttonSetState == LOW);
}

void setButton() {
  buttonRightState = getDigitalButton(buttonRightPin);
  buttonSetState = getDigitalButton(buttonSetPin);
  buttonMinusState = getDigitalButton(buttonMinusPin);
  buttonPlusState = getDigitalButton(buttonPlusPin);
}

void screenSelect() {
  if (buttonRightState == HIGH) {
    // Serial.println("+");
    screenSelectCurrent += 1;
  }
  if (screenSelectCurrent >= screenSelectSize) {
    screenSelectCurrent = 0;
  }
}

bool getDigitalButton(int pin) {
  int state = digitalRead(pin);
  if (state == HIGH) {
    setLastActivity();
    int afterState = HIGH;
    while (afterState == HIGH) {
      afterState = digitalRead(pin);
      delay(10);
    }
  }
  if (!backlightLast) {
    return LOW;
  }
  return state;
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
