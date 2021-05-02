/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <WiFi.h>
#include "time.h"

#define DEBUG true

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


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


char bluetoothMode=0;
  //0-map data to buttons
  //1-map data to wifi_ssid
  //2-map data to wifi_password
std::string wifi_ssid="";
std::string wifi_password="";
long gmtOffsetSeconds=-4*3600;

//temparary function for testing
//has same signature ase the actual function
void handleButton(int bNum){
  if(DEBUG)Serial.printf("Button Presed: %d\n",bNum);
  if(bNum==0) bluetoothMode=1;
}

//will connect and get the time
//true if sucsefull
//false if failure
boolean wifiTimeSet(){
  //still needs to decide on the gmtOffset
  if(DEBUG)Serial.printf("Connecting to %s ", wifi_ssid);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
      //may need to protect from failures
      if(WiFi.status() == WL_CONNECTION_LOST){
        if(DEBUG)Serial.print(" Connection Failed\n");
        return false;
      }
      delay(500);
      if(DEBUG)Serial.print(".");
  }
  if(DEBUG)Serial.println(" CONNECTED");
  //get the time from the web
  //need to fix daylight savings
  configTime(gmtOffsetSeconds, 0, "pool.ntp.org","time.nist.gov");
  //check if time was recived
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    if(DEBUG)Serial.println("Failed to obtain time");
    return false;
  }
  //debug display time
  if(DEBUG)Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}


//function to be called when data is recived
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      //handel the data
      if (rxValue.length() > 0) {
        if(DEBUG)Serial.println(rxValue.c_str());
        switch(bluetoothMode){
          case 0:
            //looking for a button press
            if(rxValue.length()==5 && rxValue.substr(0,2)=="!B"){
              //it is a button press
              //check the checksum
              //char sum=0;
              //for(int a=0;a<4;a++) sum+=rxValue[a];
              //if(sum==rxValue[4]){
                //checksum is valid
                //work only on button press
                if(rxValue[3]=='0'){
                  //figure out wich button
                  switch(rxValue[2]){
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
              //}
              
            }
            break;
          case 1:
            wifi_ssid=rxValue.substr(0,rxValue.length()-1);
            bluetoothMode=2;
            break;
          case 2:
            wifi_password=rxValue.substr(0,rxValue.length()-1);
            bluetoothMode=0;
            bool wifiRet=wifiTimeSet();
            if(DEBUG)Serial.printf("wifiTimeSet return: %s\n",wifiRet?"True":"False");
            break;
        }
        if(DEBUG)Serial.printf("Mode: %d\n",bluetoothMode);
        if(DEBUG)Serial.printf("SSID: %s\n",wifi_ssid.c_str());
        if(DEBUG)Serial.printf("Password: %s\n",wifi_password.c_str());
        if(DEBUG)Serial.print("--------\n");
        /*Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");*/
      }
    }
};

void bluetoothSetup(){
  // Create the BLE Device
  BLEDevice::init("ESPtest");

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

void bluetoothConnectionCheck(){
  // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        //if(DEBUG)Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

void setup() {
  if(DEBUG)Serial.begin(115200);
  bluetoothSetup();
  if(DEBUG)Serial.println("Waiting for client...");
}

void loop() {

    /*if (deviceConnected) {
        pTxCharacteristic->setValue(&txValue, 1);
        pTxCharacteristic->notify();
        txValue++;
		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
	}*/

    bluetoothConnectionCheck();
}
