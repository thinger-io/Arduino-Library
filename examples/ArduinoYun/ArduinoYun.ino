#include <YunClient.h>
#include <ThingerYun.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

ThingerYun thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
const int ledPin =  13;

void setup() {
  pinMode(ledPin, OUTPUT);

  // initialize bridge
  Bridge.begin();

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(ledPin);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}
