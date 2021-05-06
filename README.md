# LED Pannel Clock
## Overview
This project uses a light pannels and an ESP32 module to control it. The main feture is the ability to mimic daylight durring an alram. This is less intrucive than a traditional alram. There is a buzzer for providing a backup alarm. The Bluetooth low energy and WiFi capabilities of the ESP32 enable remote control via the [Adafruit Bluefruit LE Connect app](https://learn.adafruit.com/bluefruit-le-connect) and getting the current time from the internet.
## Hardware
### Componant List
* ESP32 module
* LED pannel
* 4 buttons
* 4 10k resistors
* Class D audio Amp
* Speaker
### Pin Connections
For Button functionality see the [Controls](##Controls) section.
ESP32 Pin Name | Componant
-------------|-------------
pin 1 | whatever
pin 2 | next
----|-----
## Software
The software is built using the [Arduino IDE](https://www.arduino.cc/en/software). The Bluetooth and WiFi functionality comes from the [ESP32 library](https://github.com/espressif/arduino-esp32). BLuetooth connectivity was derived from [BLE_uart example code](https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE/examples/BLE_uart). WiFifi time setting was derived from the [Simple Time example](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino). The LED pannel is controled using the [SmartMatrix](https://github.com/pixelmatix/SmartMatrix) library.
## Controls
### Button Functions
Buttons are numbered zero through 3 in software.
Function | Button Number | Number On App
----|-----|-----
fun0 | 0 | 1
fun1 | 1 | 2
fun2 | 2 | 3
fun3 | 3 | 4
### Seting up WiFi
1. Connect to the clock using the [Adafruit Bluefruit LE Connect app](https://learn.adafruit.com/bluefruit-le-connect)
2. Go to the UART terminal on the app
3. On the clock push the "Time Set" button to pull up the main menue
4. use the "Up" and "Down" buttons to navigate to the "auto set" screen
5. Select "auto set"
6. Folow the directions on the app and on the clock display (they are the same) to enter your wiFi SSID and passowrd
7. Select your standard timezone as its diffrence from GMT
  * Example: Eastern Stanrd Time (EST) is GMT -5
8. Select weather you are in an area the observes Daylight Saving Time (DST)
## Ideas for future expantion
* Suport more time zones
  * Currently only timezones that ate a whole number of hours from GMT are availible. Some regions use a fractional number hours as an offset from GMT.
  * The abreviated names of the timezones could be added so user dose not have to know (or look up) their region's offest.
* Save and reload settings on restart
  * This would write the WifI connetion and timezone information to the EEPROM so that the clock can automaticly connect to WiFi and get the time fom an NTP server.
* Battery backup
  * A battery would be added to power the clock if the primary wall power was interupted.
* Auto time zone detection
  * The clock would uses its IP address to determine its current timezone. This timezone whould the be used to ge the local time form the NTP sercers.
* Color control
  * The color of the text on the LED pannel could be set using the color picker feture of the adafuit app
* Multiple alarms
  * Add multiple alarms
  * Add reoccurring alarms
* Larger display
  * Increase the dalight simulation effect
  * Better menu navigation
  * More information like the day of the week
* Custom control app
  * This would give a more polished look and make it easer to use.
* Custom PCB and case
  * Better physical aperiance
  * Cleaner wiring
  * Better button layout and labeling
