#include <ThingerLinkItOneGPRS.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

ThingerLinkItOneGPRS thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // SIM unlock using a PIN is not supported by LinkItOne. Remove PIN from SIM before use.

  // Set your GPRS APN if it is not provided automatically by your SIM
  //thing.set_apn("orangeworld", "orange", "orange");

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