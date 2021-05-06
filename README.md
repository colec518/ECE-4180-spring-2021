# LED Pannel Clock
## Overview
This project uses a light panels and an ESP32 module to control it. The main feture is the ability to mimic daylight durring an alram. This is less intrucive than a traditional alram. There is a buzzer for providing a backup alarm. The Bluetooth low energy and WiFi capabilities of the ESP32 enable remote control via the [Adafruit Bluefruit LE Connect app](https://learn.adafruit.com/bluefruit-le-connect) and getting the current time from the internet.
## Hardware
### Component List
* ESP32 module
* HUB75 RGB LED matrix (32x16)
* 4 buttons
* 4 10k resistors
* Class D audio Amp
* Speaker
### Pin Connections
For Button functionality see the [Controls](##Controls) section.
Function | ESP32 Pin | Notes
-------------|-------------|-------------
R Channel 1 | 2
G Channel 1 | 15
B Channel 1 | 4
R Channel 2 | 16
G Channel 2 | 27
B Channel 2 | 17
Row Select A | 5
Row Select B | 18
Row Select C | 19
Row Select D | 21 | not used on 16 row matrix
Row Select E | 12 | not used on 16 row matrix
Matrix Clk | 22
Matrix Data Latch | 26
Matrix Output Enable | 25
Button 1 | 34 | pull-up resistor required
Button 2 | 35 | pull-up resistor required
Button 3 | 36 | pull-up resistor required
Button 4 | 39 | pull-up resistor required
Speaker Output | 23 | connect to input of amplifier
## Software
The software is built using the [Arduino IDE](https://www.arduino.cc/en/software). The Bluetooth and WiFi functionality comes from the [ESP32 library](https://github.com/espressif/arduino-esp32). BLuetooth connectivity was derived from [BLE_uart example code](https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE/examples/BLE_uart). WiFifi time setting was derived from the [Simple Time example](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino). The LED pannel is controled using the [SmartMatrix](https://github.com/pixelmatix/SmartMatrix) library.
## Controls
### Button Functions
Buttons are numbered zero through 3 in software.
Function | Button Number | Number On App
----|-----|-----
left | 0 | 1
right | 1 | 2
down | back | 2 | 3
up | enter | 3 | 4
### Setting The Time Maunaly
1. Press the enter Button to enter the the main menu
2. Select the "edit time" option using the "enter" button
3. Use the "up" and "down" button to adjust the value and the "Left" and "Right" buttons to change which value is being edited
4. To confirm, move the selection off the screen (e.g. pressing left when the leftmost option is selected)
### Setting The Date Manualy
1. Press the enter Button to enter the the main menu
2. Use the "Left" and "Right" keys to move over to the "edit data" menu option.
3. Select the "edit data" option using the "enter" button
4. Use the "Up" and "Down" button to adjust the value and the "Left" and "Right" buttons to change which value is being edited
5. To confirm the date move the selection of the right of the sceen by hitting the "Right" button when the years are being selected for editing
### Setting The Alarm
The same method as setting the time, but use the "set alarm" menue option insted of the "set time" option. The time set for the alarm is when the visual part of the alarm is started. The secondary auditory alarm will sart 5 minues after the time specified.
### Seting Up WiFi for Auto Time Set
1. Connect to the clock using the [Adafruit Bluefruit LE Connect app](https://learn.adafruit.com/bluefruit-le-connect)
2. Go to the UART terminal on the app
3. select "auto set" in the menu or send "w" in the UART terminal
4. Folow the directions on the app and on the clock display (they are the same) to enter your wiFi SSID and passowrd
5. Select your standard timezone as its diffrence from GMT
  * Example: Eastern Stanrd Time (EST) is GMT -5
6. Select weather you are in an area the observes Daylight Saving Time (DST)
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
