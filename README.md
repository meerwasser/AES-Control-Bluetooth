# AES-Control

AES-signal-generator for camping-refrigerators controlled by voltage and running time

Description
The device is intended for extended operation of an AES-controlled refrigerator via the 12V on-board network. Depending on the selected target values, it sends an AES signal to the corresponding output.

The device has 3 connections:
1. Board voltage
2. Mass
3. AES control output 

The push-button is used to power on the device. No it is possible to choose on the screen whether the AES signal should be switched off after a certain time or when a certain minimum voltage is reached.
When the switch-off condition is reached, the AES signal is cut off and the device is switched off by disconnecting a self-holding circuit so that it no longer consumes any power 

The device software has protective shutdowns according to internally defined maximum times and minimum voltages.

Components: 

ESP32 NodeMCU Module WLAN WiFi
Development Board mit CP2102
(Nachfolgermodell zum ESP8266) kompatibel
mit Arduino - 3x ESP32 
https://www.az-delivery.de/collections/az-specials/products/az-touch-wandgehauseset-mit-touchscreen-fur-esp8266-und-esp32

ESP32 NodeMCU Module WLAN WiFi Development Board mit CP2102 (Nachfolgermodell zum ESP8266) kompatibel mit Arduino
https://www.az-delivery.de/products/esp32-developmentboard

ULN 2003A Seven-Darlington-Arrays, DIP-16
https://www.reichelt.de/seven-darlington-arrays-dip-16-uln-2003a-p22069.html?CCOUNTRY=445&LANGUAGE=de&trstct=pos_0&nbc=1&&r=1

ADS1115 4-Kanal 16 Bit AD Wandler Breakout Board EAN 4251266703372
https://www.berrybase.de/neu/ads1115-4-kanal-16-bit-ad-wandler-breakout-board#

DIL-Miniaturrelais HJR-4102 12V, 1 Wechsler 5A
https://www.reichelt.de/de/de/index.html?ACTION=446&LA=446&nbc=1&q=hjr-4102-l%2012v%20

DIL-Miniaturrelais HJR-4102 5V, 1 Wechsler 5A
https://www.reichelt.de/de/de/index.html?ACTION=446&LA=446&nbc=1&q=hjr-4102-l%205v

Widerstände:
4,7 kΩ
22 kΩ

Taster

Changes in branch Android-Link

Minor Bugfixes:
tension-limits

Connection to Android-Device with variables to Android:

1. actual tension
2. tension limit
3. minutes left
4. state AES-Port

Android to ESP:

1. tension limit
2. minutes left
3. state AES-Port

Bluetooth-Code based on:
http://www.martyncurrey.com/android-mit-app-inventor-auto-connect-to-bluetooth/
and
https://github.com/mo-thunderz/Esp32BluetoothApp
