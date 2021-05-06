#include <MatrixHardware_ESP32_V0.h>
#include <SmartMatrix.h>
#include <time.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

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
const rgb24 sunlightColor = {0xff, 0xca, 0x7c};

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
time_t alarmTime = 0;
struct tm *currentTimeInfo;
bool alarmOn = 0;

int lastMillis = 0;
int alarmStartMillis = 0;

int alarmLightPeriod = 60 * 1000;
int fullBrightnessTime = alarmLightPeriod / 5;

////////////////////////////////////////////////////////////////////////////////
//USER INTERFACE VARIABLES
////////////////////////////////////////////////////////////////////////////////

//global variable for tracking the program state
//this is used to determine what to display and how to hanle inputs
int programState = 0;
#define DISPLAY_TIME 0
#define MENU 1
#define SET_TIME 2
#define SET_DATE 3
#define SET_ALARM 4
#define ALARM_REACHED 5
#define CONNECT_BLUETOOTH 6
#define GET_WIFI_INFO 7
#define SET_TIMEZONE 8
#define SET_DAYLIGHT 9

//global variable for the current menu selection
int menuSelection = 0;
//Constans for the name of menu items
#define M_SET_TIME 0
#define M_SET_DATE 1
#define M_SET_ALARM 2
#define M_SET_AUTO 3

//The number of menu options
#define M_NUM_OPTIONS 4

//gloabl variable ot determine which aprt of the time or deate is being edited
int currentDigit = 0;
//constanst fot labling the possibilities
#define SECOND 0
#define MINUTE 1
#define HOUR 2
#define DAY 3
#define MONTH 4
#define YEAR 5

//variables for keeping track of the the beeps when pushing buttons
bool beepPlaying = 0;
int beepTime = 0;

////////////////////////////////////////////////////////////////////////////////
//WIFI/BLUETOOTH VARIABLES
////////////////////////////////////////////////////////////////////////////////

#define DEBUG false

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

//gloabal variable to determine how the data from bluetooth should be interpreted
char bluetoothMode = 0;
//0-map data to buttons
//1-map data to wifi_ssid
//2-map data to wifi_password

//wifi login information
std::string wifi_ssid = "";
std::string wifi_password = "";

//timezone information
long gmtOffsetHours = -4;
bool daylightSavingsEnabled=true;

////////////////////////////////////////////////////////////////////////////////
//Program Start
////////////////////////////////////////////////////////////////////////////////
void setup() {
  //debug conesle setup
  Serial.begin(115200);

  //set all buttons for input
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT);
    buttons[i] = {buttonPins[i], 1, 1, 0, 0};
  }

  //set up the Buzzer
  //if opperates at a fixed frequency and is driven by PWM
  ledcSetup(0, 1000, 16);
  ledcAttachPin(23, 0);
  ledcWrite(0, 0);

  //setupt the graphics layers
  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer);
  matrix.addLayer(&indexedLayer);
  matrix.begin();

  //set the brightness
  matrix.setBrightness(defaultBrightness);

  //provide inforamion about the layers
  scrollingLayer.setOffsetFromTop(defaultScrollOffset);

  backgroundLayer.enableColorCorrection(true);
  backgroundLayer.setFont(font5x7);

  lastMillis = millis();

  bluetoothSetup();

}

void loop() {
  //struct tm *timeInfo;
  //timeInfo = localtime(&currentTime);

  //check if the beep when button press occurs need to be stoped
  if (beepPlaying && millis() - beepTime > 20) {
    stopBeep();
  }

  //update the time
  if (programState != SET_TIME && programState != SET_DATE && millis() - lastMillis > 1000) {
    lastMillis = millis();
    currentTime++;
  }

  //Start of switch tatement that determines what should be displayed
  switch (programState) {
    case DISPLAY_TIME:
      currentTimeInfo = localtime(&currentTime);

      //check if alrm need to be trigered
      if (alarmOn && currentTime >= alarmTime) {
        programState = ALARM_REACHED;
        alarmStartMillis = millis();
      }

      //draw the main cock
      drawTimeDate(currentTimeInfo);
      break;
    //handle the basic menu
    case MENU:
      drawMenu();
      break;
    //handle the basic set functions
    case SET_TIME:
      drawEditTime(currentTimeInfo);
      break;
    case SET_DATE:
      drawEditDate(currentTimeInfo);
      break;
    case SET_ALARM:
      drawEditTime(currentTimeInfo);
      break;
    //alarm state
    case ALARM_REACHED:
      handleAlarm();
      break;
    //auto time set displays
    //bluetooth and wifi screens 
    case CONNECT_BLUETOOTH:
      drawText("connect","Bluetooth");
      break;
    case GET_WIFI_INFO:
      if(bluetoothMode==2)drawText("WiFi","Password");
      else drawText("WiFi","SSID");
      break;
    //timezone and daylight savings displays
    case SET_TIMEZONE:
      drawTimeZoneEdit();
      break;
    case SET_DAYLIGHT:
      drawDaylightEdit();
  }
  //check if any of the buttons need to be updated
  updateButtons();
  //check if bluetooth is still connected
  bluetoothConnectionCheck();
}

////////////////////////////////////////////////////////////////////////////////
//BUTTON FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

//check button inputs and update
void updateButtons() {
  //go through each of the buttons and perform debouncing
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
  //start the beep head when a button is presed
  startBeep();
  //gian switch based on program state
  switch (programState) {
    case DISPLAY_TIME:
      //if the munue button is presed on the main screen then go to the menu select screen
      if (bNum == 3) {
        menuSelection = 0;
        programState = MENU;
      }
      break;

    case MENU:
      switch (bNum) {
        //alow the user to move through the posible menu options
        case 0:
          if (menuSelection == 0) menuSelection = M_NUM_OPTIONS - 1;
          else menuSelection--;
          break;
        case 1:
          if (menuSelection == (M_NUM_OPTIONS - 1)) menuSelection = 0;
          else menuSelection++;
          break;
        //beak out of the menu
        case 2:
          programState = DISPLAY_TIME;
          break;
        case 3:
          //decide what to do when an option is selected
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
            case M_SET_ALARM:
              currentDigit = HOUR;
              programState = SET_ALARM;

              //check if the alarm need to be configured for the first time
              if (alarmTime == 0) {
                currentTimeInfo = localtime(&currentTime);
                currentTimeInfo->tm_hour = 0;
                currentTimeInfo->tm_min = 0;
                currentTimeInfo->tm_sec = 0;
                alarmTime = mktime(currentTimeInfo);
              }

              currentTimeInfo = localtime(&alarmTime);
              break;
            case M_SET_AUTO:
              bluetoothMode=1;
              programState=CONNECT_BLUETOOTH;
              break;
          }
      }
      break;
    case SET_TIME:
      switch (bNum) {
        case 0:
          //check if the time should be saved and the display should go back to the basic clock screen
          if (currentDigit == HOUR) {
            currentTime = mktime(currentTimeInfo);
            updateAlarmTime();
            programState = DISPLAY_TIME;
          }
          //if not the move the cursor to the hours position
          else {
            currentDigit = HOUR;
          }
          break;
        case 1:
          //check if the time should be saved and the display should go back to the basic clock screen
          if (currentDigit == MINUTE) {
            currentTime = mktime(currentTimeInfo);
            updateAlarmTime();
            programState = DISPLAY_TIME;
          }
          //if not the move the cursor to the minuets position 
          else {
            currentDigit = MINUTE;
          }
          break;
        case 2:
          //decrement the currently selected item
          if (currentDigit == HOUR) currentTimeInfo->tm_hour = currentTimeInfo->tm_hour - 1;
          else if (currentDigit == MINUTE) currentTimeInfo->tm_min = currentTimeInfo->tm_min - 1;
          mktime(currentTimeInfo);
          break;
        case 3:
          //increment the currently selected item
          if (currentDigit == HOUR) currentTimeInfo->tm_hour = currentTimeInfo->tm_hour + 1;
          else if (currentDigit == MINUTE) currentTimeInfo->tm_min = currentTimeInfo->tm_min + 1;
          mktime(currentTimeInfo);
          break;
      }
      break;
    case SET_DATE:
      switch (bNum) {
        case 0:
          //check if the date should be saved and the display should go back to the basic clock screen
          if (currentDigit == MONTH) {
            currentTime = mktime(currentTimeInfo);
            updateAlarmTime();
            programState = DISPLAY_TIME;
          }
          //change the selection
          else if (currentDigit == DAY) currentDigit = MONTH;
          else currentDigit = DAY;
          break;
        case 1:
          //check if the date should be saved and the display should go back to the basic clock screen
          if (currentDigit == YEAR) {
            currentTime = mktime(currentTimeInfo);
            updateAlarmTime();
            programState = DISPLAY_TIME;
          }
          //change the selection
          else if (currentDigit == DAY) currentDigit = YEAR;
          else currentDigit = DAY;
          break;
        case 2:
          //decrement the currently selected item
          if (currentDigit == MONTH) currentTimeInfo->tm_mon = currentTimeInfo->tm_mon - 1;
          else if (currentDigit == DAY) currentTimeInfo->tm_mday = currentTimeInfo->tm_mday - 1;
          else if (currentDigit == YEAR) currentTimeInfo->tm_year = currentTimeInfo->tm_year - 1;
          mktime(currentTimeInfo);
          break;
        case 3:
          //increment the currently selected item
          if (currentDigit == MONTH) currentTimeInfo->tm_mon = currentTimeInfo->tm_mon + 1;
          else if (currentDigit == DAY) currentTimeInfo->tm_mday = currentTimeInfo->tm_mday + 1;
          else if (currentDigit == YEAR) currentTimeInfo->tm_year = currentTimeInfo->tm_year + 1;
          mktime(currentTimeInfo);
          break;
      }
      break;
    case SET_ALARM:
      switch (bNum) {
        case 0:
          //check if the time should be saved and the display should go back to the basic clock screen
          if (currentDigit == HOUR) {
            alarmTime = mktime(currentTimeInfo);
            updateAlarmTime();
            alarmOn = 1;
            programState = DISPLAY_TIME;
          }
          //change the selection
          else {
            currentDigit = HOUR;
          }
          break;
        case 1:
          //check if the time should be saved and the display should go back to the basic clock screen
          if (currentDigit == MINUTE) {
            alarmTime = mktime(currentTimeInfo);
            updateAlarmTime();
            alarmOn = 1;
            programState = DISPLAY_TIME;
          }
          //change the selection
          else {
            currentDigit = MINUTE;
          }
          break;
        case 2:
          //decrement the currently selected item
          if (currentDigit == HOUR) currentTimeInfo->tm_hour = currentTimeInfo->tm_hour - 1;
          else if (currentDigit == MINUTE) currentTimeInfo->tm_min = currentTimeInfo->tm_min - 1;
          mktime(currentTimeInfo);
          break;
        case 3:
          //increment the currently selected item
          if (currentDigit == HOUR) currentTimeInfo->tm_hour = currentTimeInfo->tm_hour + 1;
          else if (currentDigit == MINUTE) currentTimeInfo->tm_min = currentTimeInfo->tm_min + 1;
          mktime(currentTimeInfo);
          break;
      }
      break;
    //handel the end of an alarm
    case ALARM_REACHED:
      updateAlarmTime();
      programState = DISPLAY_TIME;
      ledcWrite(0, 0);
      matrix.setBrightness(defaultBrightness);
      break;
    //prompt the user to connect to bluetooth
    case CONNECT_BLUETOOTH:
      if(deviceConnected){
        programState=GET_WIFI_INFO;
        bluetoothAskForSsid();
      }
      if(bNum==2)programState=DISPLAY_TIME;
      break;
    //give prompt the user on how to put in the WiFi info
    case GET_WIFI_INFO:
      switch(bluetoothMode){
        case 0://has password and SSID
          //waiting for connectiion sucess or failure;
          if(WiFi.status()==WL_CONNECTED)programState=SET_TIMEZONE;
          break;
        case 1://getting SSID
          //noting to do here
          break;
        case 2://getting Password
          //check back button
          if(bNum==2)bluetoothAskForSsid();
          break;
      }
    //handel the select timezone screen
    case SET_TIMEZONE:
      switch(bNum){
        case 0: //decrease
          gmtOffsetHours=(gmtOffsetHours<=-12)?-12:gmtOffsetHours-1;
          break;
        case 1: //increase
          gmtOffsetHours=(gmtOffsetHours>=12)?12:gmtOffsetHours+1;
          break;
        case 2:
          programState=deviceConnected?GET_WIFI_INFO:DISPLAY_TIME;
          break;
        case 3: //enter
          programState=SET_DAYLIGHT;
          break;
      }
      break;
    //handel the daylight savings tiem select screen
    case SET_DAYLIGHT:
      switch(bNum){
        case 0: //togel
        case 1: //togel
          daylightSavingsEnabled=!daylightSavingsEnabled;
          break;
        case 2:
          programState=SET_TIMEZONE;
          break;
        case 3: //enter
          //perform the timeset
          if(wifiTimeSet()) currentTime=time(0);
          programState=DISPLAY_TIME;
          break;
      }
  }
}

//starts the beep sound
void startBeep() {
  beepPlaying = 1;
  beepTime = millis();
  //PWM with 50% duty cysle
  ledcWrite(0, 65535 / 2);
}

//stops the beep sound
void stopBeep() {
  beepPlaying = 0;
  //PWM with 0% duty cycle
  ledcWrite(0, 0);
}

////////////////////////////////////////////////////////////////////////////////
//CLOCK FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void handleAlarm() {
  int timeSinceAlarm = millis() - alarmStartMillis;

  //handel increasing the bightness at the begining of the alarm
  //this creates a ramp up effect
  if (timeSinceAlarm <= fullBrightnessTime) {
    int brightness = 255 * (float(timeSinceAlarm) / float(fullBrightnessTime));

    matrix.setBrightness(brightness);
  }

  //this contols the backup alarm
  if (timeSinceAlarm >= alarmLightPeriod) {
    if (millis() % 500 > 200) {
      //turn buzzer on
      ledcWrite(0, 65535 / 2);
    }
    else {
      //turn buzzer off
      ledcWrite(0, 0);
    }
  }

  //handle the graphics display of the sunrise color
  backgroundLayer.fillScreen(sunlightColor);
  backgroundLayer.swapBuffers();
}

//update the alarm so that it occurs at the same time the next day
void updateAlarmTime() {
  currentTimeInfo = localtime(&alarmTime);
  int alarmHour = currentTimeInfo->tm_hour;
  int alarmMinute = currentTimeInfo->tm_min;
  int alarmSecond = currentTimeInfo->tm_sec;

  currentTimeInfo = localtime(&currentTime);
  if (currentTimeInfo->tm_hour < alarmHour || (currentTimeInfo->tm_hour == alarmHour && currentTimeInfo->tm_min < alarmMinute)) {
    currentTimeInfo->tm_mday = currentTimeInfo->tm_mday;
  }
  else {
    currentTimeInfo->tm_mday = currentTimeInfo->tm_mday + 1;
  }
  currentTimeInfo->tm_hour = alarmHour;
  currentTimeInfo->tm_min = alarmMinute;
  currentTimeInfo->tm_sec = alarmSecond;

  alarmTime = mktime(currentTimeInfo);
  currentTimeInfo = localtime(&currentTime);
}

////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

//display the current time with the date below it on the matrix
void drawTimeDate(tm *timeInfo) {
  char timeText[32];
  char dateText[32];
  
  //format the time and date
  strftime(timeText, 32, "%H:%M", timeInfo);
  strftime(dateText, 32, "%m/%d/%y", timeInfo);
  
  //display the time and date
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font6x10);
  backgroundLayer.drawString(0, 0, {255, 255, 255}, timeText);
  backgroundLayer.setFont(font3x5);
  backgroundLayer.drawString(0, 10, {255, 255, 255}, dateText);
  backgroundLayer.swapBuffers();
}

void drawMenu() {
  //clear screen
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font5x7);
  //backgroundLayer.drawString(0, 0, {255, 255, 255}, menuText[menuSelection]);

  //change the menu text based on the current option beeing looked at
  switch (menuSelection) {
    case 0:
      backgroundLayer.drawString(0, 0, {255, 255, 255}, "edit");
      backgroundLayer.drawString(0, 8, {255, 255, 255}, "time");
      break;
    case 1:
      backgroundLayer.drawString(0, 0, {255, 255, 255}, "edit");
      backgroundLayer.drawString(0, 8, {255, 255, 255}, "date");
      break;
    case 2:
      backgroundLayer.drawString(0, 0, {255, 255, 255}, "set");
      backgroundLayer.drawString(0, 8, {255, 255, 255}, "alarm");
      break;
    case 3:
      backgroundLayer.drawString(0, 0, {255, 255, 255}, "auto");
      backgroundLayer.drawString(0, 8, {255, 255, 255}, "set");
      break;
  }

  backgroundLayer.swapBuffers();
}

void drawEditTime(tm *timeInfo) {
  char timeText[32];

  //format the time
  strftime(timeText, 32, "%H:%M", timeInfo);

  //draw the time
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font6x10);
  backgroundLayer.drawString(0, 2, {255, 255, 255}, timeText);

  //draw the selection bar
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

  //format the date
  strftime(dateText, 32, "%m/%d/%y", timeInfo);

  //display the date
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font3x5);
  backgroundLayer.drawString(0, 5, {255, 255, 255}, dateText);

  //draw the selection bar
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
//simple function to draw two lines of text
void drawText(String top,String bot){
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font5x7);
  backgroundLayer.drawString(0, 0, {255, 255, 255}, top.c_str());
  backgroundLayer.drawString(0, 8, {255, 255, 255}, bot.c_str());
  backgroundLayer.swapBuffers();
}
//function for drawing the timezone edit screen
void drawTimeZoneEdit(){
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font3x5);
  char text[32];
  //format the text so that the numebr always has a '+' or '-' sign as is tipical with timezones
  sprintf(text,"GMT%c%d",(gmtOffsetHours<0)?'-':'+',(gmtOffsetHours<0)?-gmtOffsetHours:gmtOffsetHours);
  //draw the text
  backgroundLayer.drawString(0, 0, {255, 255, 255}, text);
  //draw the underline
  backgroundLayer.drawString(0, 6, {255, 255, 255}, "   ___");
  backgroundLayer.swapBuffers();
}
void drawDaylightEdit(){
  backgroundLayer.fillScreen({0, 0, 0});
  backgroundLayer.setFont(font3x5);
  //display based on if user says "yes" or "no" to daylight savings time
  if(daylightSavingsEnabled){
    //draw the text portion
    backgroundLayer.drawString(0, 0, {255, 255, 255}, "DST Yes");
    //draw the underline
    backgroundLayer.drawString(0, 6, {255, 255, 255}, "    ___");
  }
  else{
    //draw the text portion
    backgroundLayer.drawString(0, 0, {255, 255, 255}, "DST No");
    //draw the underline
    backgroundLayer.drawString(0, 6, {255, 255, 255}, "    __");
  }
  backgroundLayer.swapBuffers();
}

////////////////////////////////////////////////////////////////////////////////
//WIFI/BLUETOOTH FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

//callback for bluetooth connection changes
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

//callback when data is recived
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      //handel the data
      if (rxValue.length() > 0) {
        if (DEBUG)Serial.println(rxValue.c_str());
        switch (bluetoothMode) {
          case 0:
            //looking for a button press
            if (rxValue.length() == 5 && rxValue.substr(0, 2) == "!B") {
              //it is a button code
              //work only on button press
              if (rxValue[3] == '0') {
                //figure out wich button
                switch (rxValue[2]) {
                  case '1':
                    handleButton(0);
                    break;
                  case '2':
                    handleButton(1);
                    break;
                  case '3':
                    handleButton(2);
                    break;
                  case '4':
                    handleButton(3);
                    break;
                }
              }

            }
            break;
          case 1:
            //put data in the SSID sting
            wifi_ssid = rxValue.substr(0, rxValue.length() - 1);
            //ask for pasword
            pTxCharacteristic->setValue("Password:");
            pTxCharacteristic->notify();
            bluetoothMode = 2;
            break;
          case 2:
            //put data in the password string
            wifi_password = rxValue.substr(0, rxValue.length() - 1);
            //go back to default bluetooth mode
            bluetoothMode = 0;
            //connect to the WiFi
            bool wifiRet = wifiConnect();
            if (DEBUG)Serial.printf("wifiConnect return: %s\n", wifiRet ? "True" : "False");
            break;
        }
        //debug statements
        if (DEBUG)Serial.printf("Mode: %d\n", bluetoothMode);
        if (DEBUG)Serial.printf("SSID: %s\n", wifi_ssid.c_str());
        if (DEBUG)Serial.printf("Password: %s\n", wifi_password.c_str());
        if (DEBUG)Serial.print("--------\n");
      }
    }
};

//function for asking for the SSID
void bluetoothAskForSsid(){
  pTxCharacteristic->setValue("Please enter your WiFi information:\n");
  pTxCharacteristic->notify();
  pTxCharacteristic->setValue("SSID:\n");
  pTxCharacteristic->notify();
  bluetoothMode=1;
}


//will atempt to connect to the WiFi
//true if sucsefull
//false if failure
boolean wifiConnect(){
  if(DEBUG)Serial.printf("Connecting to %s ", wifi_ssid.c_str());
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
      //may need to protect from failures
      if(WiFi.status() == WL_CONNECTION_LOST){
        if(DEBUG)Serial.print(" Connection Failed\n");
        pTxCharacteristic->setValue("Connection Failed\n");
        pTxCharacteristic->notify();
        return false;
      }
      delay(500);
      if(DEBUG)Serial.print(".");
  }
  if(DEBUG)Serial.println(" CONNECTED");
  pTxCharacteristic->setValue("Connected\n");
  pTxCharacteristic->notify();
  return true;
}
//will atempt to get time from NTP server
//true if sucsefull
//false if failure
boolean wifiTimeSet(){
  //get the time from the web
  configTime(gmtOffsetHours*3600, daylightSavingsEnabled?3600:0, "pool.ntp.org","time.nist.gov");
  //check if time was recived
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    if(DEBUG)Serial.println("Failed to obtain time");
    pTxCharacteristic->setValue("Failed to obtain time\n");
    pTxCharacteristic->notify();
    return false;
  }
  //debug display time
  if(DEBUG)Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}

//reainging code taken from the BLE_UART example that
//comes with the ESP32 arduino package
void bluetoothSetup() {
  // Create the BLE Device
  BLEDevice::init("ESP32 Clock");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                          );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
}

void bluetoothConnectionCheck() {
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    if(DEBUG)Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    //nothing to do on connection
    oldDeviceConnected = deviceConnected;
  }
}
