// Arduino UNO Q: the sketch runs on the MCU and connects through the Linux side
// (requires Arduino_RouterBridge from Library Manager). Configure the network on Linux
// first, i.e. with the Arduino App Lab or "nmcli dev wifi connect <ssid> password <password>"
// The debug output goes to the Monitor of the Arduino App Lab / Arduino IDE

#define THINGER_SERIAL_DEBUG

#include <ThingerUnoQ.h>
#include "arduino_secrets.h"

ThingerUnoQ thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // open the monitor for debugging (Serial is the Monitor of the bridge)
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}
