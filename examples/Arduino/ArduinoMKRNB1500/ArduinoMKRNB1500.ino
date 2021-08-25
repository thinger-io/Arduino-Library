#define THINGER_SERIAL_DEBUG

#include <ThingerMKRNB.h>
#include <ThingerMKRNBOTA.h>
#include "arduino_secrets.h"

ThingerMKRNB thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
// OTA seems to not work if no reset button pressed
ThingerMKRNBOTA ota(thing);

void setup() {
  // enable serial for debugging
  Serial.begin(115200);

  // optional set pin number
  thing.set_pin(PIN_NUMBER);

  // set APN
  thing.set_apn(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD);

  // set builtin led to output
  pinMode(LED_BUILTIN, OUTPUT);

  // pin control example over internet (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["time"] >> [&](pson& out){
      out = thing.getNB().getTime();
  };

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}
