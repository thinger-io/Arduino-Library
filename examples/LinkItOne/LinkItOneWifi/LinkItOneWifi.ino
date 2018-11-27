#include <ThingerLinkItOneWifi.h>

ThingerLinkItOneWifi thing("user_id", "device_id", "device_credential");

void setup() {
  thing.add_wifi("SSID", "SSID_Password");

  pinMode(2, OUTPUT);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(2);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}