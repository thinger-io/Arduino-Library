Arduino client is a library that allows connecting your IoT devices to the [thinger.io](http://thinger.io "thinger.io IoT Cloud Platform") cloud platform, or even your private server deployment. This is a library specifically designed for the Arduino IDE, so you can easily connect your devices.

It supports multiple network interfaces like Ethernet, Wifi, and GSM. It requires the latest Arduino version (at least 1.6.3).

## Installation

Download this repository as zip, and decompress it under your Arduino library folder with a proper name like thinger or thinger.io. Then start your Arduino and you will find different project examples for the Adafruit CC3000, ESP8266, Energia CC3200, and the WiFi or Ethernet shield. Just modify the user, device and device credentials (and Wifi access point if required) to start the connection of your device with the thinger.io platform. 


## TODO

This library is currently being developed and should not be integrated in any production device until it is more mature and tested. The library interface may change as it evolve to a stable version. Some things that requires attention now are:

 - Document code
 - Provide more examples and use cases
 - Test with different devices that support the Arduino IDE (Tested on CC3200 with Energia).
 - Improve the reconnection mechanism for some interfaces.

## License

<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

The class is licensed under the [MIT License](http://opensource.org/licenses/MIT):

Copyright &copy; 2015 [THINGER LTD](http://thinger.io)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.