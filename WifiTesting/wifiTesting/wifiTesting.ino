#include <WiFi.h>
#include "time.h"

char* ssid       = new char[32];//"AndroidAP1D30";
char* password   = new char[32];//"onoe9569";
//const char* ntpServer = ;
long gmtOffsetSeconds=-4*3600;

//Bluetooth
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;


void setup() {
  //setup bluetooth
  Serial.begin(115200);
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  delay(1000);
  //get the SSID
  SerialBT.write('SSID:\n');
  //temparay infinate loop to see what comes out
  unsigned char tmp;
  while(1){
    if (SerialBT.available()) {
      tmp=SerialBT.read();
      Serial.printf("%c,%X\n",tmp,tmp);
    }
    delay(20);
  }

  
  //connect to wifi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //get the time
  configTime(gmtOffsetSeconds, 0, "pool.ntp.org","time.nist.gov");

  //display the time
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void loop() {
  // put your main code here, to run repeatedly:
  //do nothing
  delay(1000);
}
