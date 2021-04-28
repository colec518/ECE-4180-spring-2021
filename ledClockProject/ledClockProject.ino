#include <MatrixHardware_ESP32_V0.h>
#include <SmartMatrix.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
//MATRIX VARIABLES
////////////////////////////////////////////////////////////////////////////////

#define COLOR_DEPTH 24                  // Choose the color depth used for storing pixels in the layers: 24 or 48 (24 is good for most sketches - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24)
const uint16_t kMatrixWidth = 32;       // Set to the width of your display, must be a multiple of 8
const uint16_t kMatrixHeight = 16;      // Set to the height of your display
const uint8_t kRefreshDepth = 36;       // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_16ROW_MOD8SCAN;   // Choose the configuration that matches your panels.  See more details in MatrixCommonHub75.h and the docs: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);        // see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

const int defaultBrightness = 100;
const int defaultScrollOffset = 6;
const rgb24 defaultBackgroundColor = {0x40, 0, 0};

////////////////////////////////////////////////////////////////////////////////
//BUTTON VARIABLES
////////////////////////////////////////////////////////////////////////////////

//struct used for button debouncing
struct button {
  int pin;
  int state;
  int lastState;
  int reading;
  int debounceTime;
};

int debounceDelay = 50;
int buttonPins[] = {35, 34, 39, 36};
button buttons[4];

////////////////////////////////////////////////////////////////////////////////
//CLOCK VARIABLES
////////////////////////////////////////////////////////////////////////////////

time_t currentTime = 1619132563;
struct tm *currentTimeInfo;
int lastMillis = 0;

/*int tempSecond = 0;
int tempMinute = 0;
int tempHour = 0;
int tempDay = 0;
int tempMonth = 0;
int tempYear = 0;*/

////////////////////////////////////////////////////////////////////////////////
//USER INTERFACE VARIABLES
////////////////////////////////////////////////////////////////////////////////

int programState = 0;
#define DISPLAY_TIME 0
#define MENU 1
#define SET_TIME 2
#define SET_DATE 3

int menuSelection = 0;
#define M_SET_TIME 0
#define M_SET_DATE 1

#define M_NUM_OPTIONS 2

int currentDigit = 0;
#define SECOND 0
#define MINUTE 1
#define HOUR 2
#define DAY 3
#define MONTH 4
#define YEAR 5

char menuText[][64] = {
  "EDIT\nTIME",
  "EDIT\nDATE"
};

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT);
    buttons[i] = {buttonPins[i], 1, 1, 0, 0};
  }

  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer);
  matrix.addLayer(&indexedLayer);
  matrix.begin();

  matrix.setBrightness(defaultBrightness);

  scrollingLayer.setOffsetFromTop(defaultScrollOffset);

  backgroundLayer.enableColorCorrection(true);
  backgroundLayer.setFont(font5x7);

  lastMillis = millis();
}

void loop() {
  //struct tm *timeInfo;
  //timeInfo = localtime(&currentTime);

  switch (programState) {
    case DISPLAY_TIME:
      if (millis() - lastMillis > 1000) {
        struct tm *timeInfo;
        timeInfo = localtime(&currentTime);
        lastMillis = millis();
        currentTime++;

        drawTimeDate(timeInfo);
      }
      break;
    case MENU:
      drawMenu();
      break;
    case SET_TIME:
      drawEditTime(currentTimeInfo);
      break;
    case SET_DATE:
      drawEditDate(currentTimeInfo);
      break;
  }

  updateButtons();
}

////////////////////////////////////////////////////////////////////////////////
//BUTTON FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

//check button inputs and update
void updateButtons() {
  for (int i = 0; i < 4; i++) {
    button *currentButton = buttons + i;
    int currentPin = currentButton->pin;

    currentButton->reading = digitalRead(currentPin);
    if (currentButton->reading != currentButton->lastState) {
      currentButton->debounceTime = millis();
    }

    if (millis() - currentButton->debounceTime > debounceDelay) {
      if (currentButton->state != currentButton->reading) {
        currentButton->state = currentButton->reading;
        if (currentButton->state == LOW) {
          handleButton(i);
        }
      }
    }
    currentButton->lastState = currentButton->reading;
  }
}

//take action based on which button has been pressed
void handleButton(int bNum) {
  switch (programState) {
    case DISPLAY_TIME:
      if (bNum == 3) {
        menuSelection = 0;
        programState = MENU;
      }
      break;

    case MENU:
      switch (bNum) {
        case 0:
          if (menuSelection == 0) menuSelection = M_NUM_OPTIONS - 1;
          else menuSelection--;
          break;
        case 1:
          if (menuSelection == (M_NUM_OPTIONS - 1)) menuSelection = 0;
          else menuSelection++;
          break;
        case 2:
          programState = DISPLAY_TIME;
          break;
        case 3:
          switch (menuSelection) {
            case M_SET_TIME:
              currentDigit = HOUR;
              programState = SET_TIME;
              currentTimeInfo = localtime(&currentTime);
              break;
            case M_SET_DATE:
              currentDigit = MONTH;
              programState = SET_DATE;
              currentTimeInfo = localtime(&currentTime);
              break;
          }
      }
      break;
    case SET_TIME:
      switch (bNum) {
        case 0:
          if (currentDigit == HOUR) {
            currentTime = mktime(currentTimeInfo);
            programState = DISPLAY_TIME;
          }
          else {
            currentDigit = HOUR;
          }
          break;
        case 1:
          if (currentDigit == MINUTE) {
            currentTime = mktime(currentTimeInfo);
            programState = DISPLAY_TIME;
          }
          else {
            currentDigit = MINUTE;
          }
          break;
        case 2:
          if (currentDigit == HOUR) currentTimeInfo->tm_hour = currentTimeInfo->tm_hour - 1;
          else if (currentDigit == MINUTE) currentTimeInfo->tm_min = currentTimeInfo->tm_min - 1;
          mktime(currentTimeInfo);
          break;
        case 3:
          if (currentDigit == HOUR) currentTimeInfo->tm_hour = currentTimeInfo->tm_hour + 1;
          else if (currentDigit == MINUTE) currentTimeInfo->tm_min = currentTimeInfo->tm_min + 1;
          mktime(currentTimeInfo);
          break;
      }
      break;
    case SET_DATE:
      switch (bNum) {
        case 0:
          if (currentDigit == MONTH) {
            currentTime = mktime(currentTimeInfo);
            programState = DISPLAY_TIME;
          }
          else if (currentDigit == DAY) currentDigit = MONTH;
          else currentDigit = DAY;
          break;
        case 1:
          if (currentDigit == YEAR) {
            currentTime = mktime(currentTimeInfo);
            programState = DISPLAY_TIME;
          }
          else if (currentDigit == DAY) currentDigit = YEAR;
          else currentDigit = DAY;
          break;
        case 2:
          if (currentDigit == MONTH) currentTimeInfo->tm_mon = currentTimeInfo->tm_mon - 1;
          else if (currentDigit == DAY) currentTimeInfo->tm_mday = currentTimeInfo->tm_mday - 1;
          else if (currentDigit == YEAR) currentTimeInfo->tm_year = currentTimeInfo->tm_year - 1;
          mktime(currentTimeInfo);
          break;
        case 3:
          if (currentDigit == MONTH) currentTimeInfo->tm_mon = currentTimeInfo->tm_mon + 1;
          else if (currentDigit == DAY) currentTimeInfo->tm_mday = currentTimeInfo->tm_mday + 1;
          else if (currentDigit == YEAR) currentTimeInfo->tm_year = currentTimeInfo->tm_year + 1;
          mktime(currentTimeInfo);
          break;
      }
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//CLOCK FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/*void loadTempTime() {
  struct tm *timeInfo;
  timeInfo = localtime(&currentTime);
  currentTimeInfo = localtime(&currentTime);

  tempSecond = timeInfo->tm_sec;
  tempMinute = timeInfo->tm_min;
  tempHour = timeInfo->tm_hour;
  tempDay = timeInfo->tm_mday;
  tempMonth = timeInfo->tm_mon;
  tempYear = timeInfo->tm_year;
}

void saveTempTime() {
  struct tm *timeInfo;
  timeInfo = localtime(&currentTime);

  timeInfo->tm_sec = tempSecond;
  timeInfo->tm_min = tempMinute;
  timeInfo->tm_hour = tempHour;
  timeInfo->tm_mday = tempDay;
  timeInfo->tm_mon = tempMonth;
  timeInfo->tm_year = tempYear;

  currentTime = mktime(timeInfo);
  currentTimeInfo = localtime(&currentTime);
}*/

////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

//display the current time with the date below it on the matrix
void drawTimeDate(tm *timeInfo) {
  char timeText[32];
  char dateText[32];

  strftime(timeText, 32, "%H:%M", timeInfo);
  strftime(dateText, 32, "%m/%d/%y", timeInfo);

  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font6x10);
  backgroundLayer.drawString(0, 0, {255, 255, 255}, timeText);
  backgroundLayer.setFont(font3x5);
  backgroundLayer.drawString(0, 10, {255, 255, 255}, dateText);
  backgroundLayer.swapBuffers();
}

void drawMenu() {
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font5x7);
  //backgroundLayer.drawString(0, 0, {255, 255, 255}, menuText[menuSelection]);

  switch(menuSelection) {
    case 0:
      backgroundLayer.drawString(0, 0, {255, 255, 255}, "edit");
      backgroundLayer.drawString(0, 8, {255, 255, 255}, "time");
      break;
    case 1:
      backgroundLayer.drawString(0, 0, {255, 255, 255}, "edit");
      backgroundLayer.drawString(0, 8, {255, 255, 255}, "date");
      break;
  }
  
  backgroundLayer.swapBuffers();
}

void drawEditTime(tm *timeInfo) {
  char timeText[32];

  strftime(timeText, 32, "%H:%M", timeInfo);

  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font6x10);
  backgroundLayer.drawString(0, 2, {255, 255, 255}, timeText);

  switch (currentDigit) {
    case HOUR:
      backgroundLayer.drawString(0, 3, {255, 255, 255}, "__");
      break;
    case MINUTE:
      backgroundLayer.drawString(0, 3, {255, 255, 255}, "   __");
      break;
  }

  backgroundLayer.swapBuffers();
}

void drawEditDate(tm *timeInfo) {
  char dateText[32];

  strftime(dateText, 32, "%m/%d/%y", timeInfo);

  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font3x5);
  backgroundLayer.drawString(0, 5, {255, 255, 255}, dateText);

  switch (currentDigit) {
    case MONTH:
      backgroundLayer.drawString(0, 6, {255, 255, 255}, "__");
      break;
    case DAY:
      backgroundLayer.drawString(0, 6, {255, 255, 255}, "   __");
      break;
    case YEAR:
      backgroundLayer.drawString(0, 6, {255, 255, 255}, "      __");
      break;
  }

  backgroundLayer.swapBuffers();
}
