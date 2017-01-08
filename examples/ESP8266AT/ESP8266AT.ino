// Can be installed from Library Manager or https://github.com/bportaluri/WiFiEsp
#include <WiFiEsp.h>
#include <ThingerESP8266AT.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

#define SSID "your_wifi_ssid"
#define SSID_PASSWORD "your_wifi_ssid_password"

#define ARDUINO_LED 13

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

ThingerESP8266AT thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  pinMode(ARDUINO_LED, OUTPUT);

  /* Notice: initialize serial for ESP8266 (it must be at 9600 if using SoftwareSerial)
   * You can change the baudrate of ESP8266 sending the command AT+UART_DEF=9600,8,1,0,0\r\n
   * If your device supports a HW Serial1, you can use 115200.
   */
  Serial1.begin(9600);

  // init ESP8266 in the additional serial port
  WiFi.init(&Serial1);
  if(WiFi.status() == WL_NO_SHIELD){
      // don't continue
      while (true);
  }

  thing.add_wifi(SSID, SSID_PASSWORD);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["led"] << digitalPin(ARDUINO_LED);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}