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
