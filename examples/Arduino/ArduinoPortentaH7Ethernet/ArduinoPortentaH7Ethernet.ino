#define THINGER_SERIAL_DEBUG

#include <ThingerMbedEth.h>
#include <ThingerPortentaOTA.h>
#include "arduino_secrets.h"

ThingerMbedEth thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerPortentaOTA ota(thing);

void setup() {
    // configure LED_BUILTIN for output
    pinMode(LED_BUILTIN, OUTPUT);

    // open serial for debugging
    Serial.begin(115200);

    // pin control example (i.e. turning on/off a light, a relay, etc)
    thing["led"] << digitalPin(LED_BUILTIN);

    // resource output example (i.e. reading a sensor value, a variable, etc)
    thing["millis"] >> outputValue(millis());

    // start thinger on its own task
    thing.start();

    // more details at http://docs.thinger.io/arduino/
}

void loop() {
    // use loop as in normal Arduino Sketch
    // use thing.lock() thing.unlock() when using/modifying variables exposed on thinger resources
    delay(1000);
}