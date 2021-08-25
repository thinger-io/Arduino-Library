#define THINGER_SERIAL_DEBUG

#include <ThingerWiFiNINA.h>
#include <ThingerWiFiNINAOTA.h>
#include "arduino_secrets.h"

// requires library Arduino_LSM6DS3 for the imu readings
#include <Arduino_LSM6DS3.h>

// cannot connect? Update WiFiNiNA and add iot.thinger.io SSL Certificate
// https://support.arduino.cc/hc/en-us/articles/360016119219

ThingerWiFiNINA thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerWiFiNINAOTA ota(thing);

void setup() {
  // configure LED_BUILTIN for output
  pinMode(LED_BUILTIN, OUTPUT);

  // open serial for debugging
  Serial.begin(115200);

  // initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // example for the built-int gyroscope
  thing["imu"] >> [](pson& out){
    float x, y, z;
    IMU.readGyroscope(x, y, z);
    out["x"] = x;
    out["y"] = y;
    out["z"] = z;
  };

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}