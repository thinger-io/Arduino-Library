Arduino client is a library that allows connecting your IoT devices to the [thinger.io](http://thinger.io "thinger.io IoT Cloud Platform") cloud platform, or even your private server deployment. This is a library specifically designed for the Arduino IDE, so you can easily connect your devices.

It supports multiple network interfaces like Ethernet, Wifi, and GSM. It requires the latest Arduino version (at least 1.6.3).

## Installation

You can easily install the libraries with the **Arduino Library Manager** searching for `thinger.io. If you need more help, please read the topic posted in our [community forum](https://community.thinger.io/t/install-thinger-io-in-arduino-ide/21 "Thinger.io Community Forum"). 

## Defining Resources

In the thinger.io platform, each device or thing can define several resources. A resource is like a function that can be executed remotely in your device in real time. This way you can actuate over your devices like turning on/off lights, or extracting information like reading a sensor value. You can take a look to the examples provided in the library to start setting your own resources. You can define several resources in the setup() method.

There are three different types of resources:

**Input**

Using the operator << pointing to the resource name represents input. In this case this function takes one parameter of type `pson` that is a variable type that can contain booleans, numbers, floats, strings, or even structured information like in a json document. The following example function will receive a boolean that will allow turning on and off the led.

```
thing["led"] << [](pson& in){
     digitalWrite(BUILTIN_LED, in ? HIGH : LOW);
};
```

**Output**

Using the operator >> pointing out of the resource name means output. As the input function, it contains one `pson` parameter, that will allow storing our output information. In this case each time the function is called it is filled with the `millis()` value (the time the device has been running), but you can put here your sensors readings.

```
thing["millis"] >> [](pson& out){
      out = millis();
};
```

**Input/Output**

Here it is used the operator = for defining a function that have both input and output. In this case the function takes to different `pson` parameters. One for input data and another one for output data. The following example will output the sum and multiplication of two different values (`value1`, and `value2`) provided as input. Notice here how the `pson` structure can store values with keys, like any `json` document. The result provides two different values in `sum` and `mult` keys, that it is translated to `json` when you access them from the API.

```
thing["in_out"] = [](pson& in, pson& out){
      out["sum"] = (long)in["value1"] + (long)in["value2"];
      out["mult"] = (long)in["value1"] * (long)in["value2"];
};
```

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