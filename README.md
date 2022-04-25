[![arduino-library-badge](https://www.ardu-badge.com/badge/thinger.io.svg?)](https://docs.thinger.io/sdk-setup)

The Arduino Client Library is an easy to use client library to connect your IoT devices to the [Thinger.io](https://thinger.io "Thinger.io IoT Cloud Platform") IoT platform. This is a library specifically designed for the Arduino IDE/Platformio, so you can easily install it in your environment and start connecting your devices within minutes.

## Supported Boards

It supports multiple network interfaces like Ethernet, Wifi, and GSM. So you can use it in several devices like the following:

* Espressif ESP8266 (OTA Support)
* Espressif ESP32 (OTA Support)
* Arduino Nano RP2040 Connect (OTA Support)
* Arduino Nano 33 IoT (OTA Support)
* Arduino Portenta H7 (OTA Support)
* Arduino MKR 1010 (OTA Support)
* Arduino MKR NB 1500 (OTA Support)
* Arduino MKR 1000 
* Arduino GSM1400 (MKRGSM)
* Arduino + Ethernet
* Arduino + Wifi
* Arduino + Adafruit CC3000
* Arduino + ENC28J60
* Arduino Yun
* Arduino + GPRS Shield
* Arduino + TinyGSM library for GSM modems using GPRS (SIM800, SIM900, AI-THINKER A6, A6C, A7, Neoway M590)
* Arduino + ESP8266 as WiFi Modem via AT commands (using TinyGSM library)
* Texas Instruments CC3200
* SeeedStudio LinkIt ONE (Both GPRS and WiFi)

## OTA (Over the Internet)

Some devices can be directly updated remotely over the Internet (OTA). Thinger.io provides a Visual Studio Code extension for the OTA process, from building the firmware, flashing over the Internet, to remotelly rebooting the device. More details [here](https://marketplace.visualstudio.com/items?itemName=thinger-io.thinger-io).

![](https://s3.eu-west-1.amazonaws.com/thinger.io.files/vscode/iot-ota.gif)

## Documentation

Please, refer to the following page for a full documentation of the Arduino Client Library.

[Arduino Client Library Documentation](http://docs.thinger.io/arduino/)

## License

<img align="right" src="https://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

The class is licensed under the [MIT License](http://opensource.org/licenses/MIT):

Copyright &copy; [Thinger.io](http://thinger.io)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.