// enable debug output over serial
#define THINGER_SERIAL_DEBUG

// define private server instance
#define THINGER_SERVER "acme.aws.thinger.io"

#include <ThingerMbedEth.h>
#include <ThingerPortentaOTA.h>
#include "arduino_secrets.h"

ThingerMbedEth thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerPortentaOTA ota(thing);

void setup() {
    // open serial for debugging
    Serial.begin(115200);

    // configure leds for output
    pinMode(LED_D0, OUTPUT);
    pinMode(LED_D1, OUTPUT);
    pinMode(LED_D2, OUTPUT);
    pinMode(LED_D3, OUTPUT);
    pinMode(LEDR, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    // configure relays for output
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
    pinMode(D2, OUTPUT);
    pinMode(D3, OUTPUT);

    // example for controlling relays and status led
    thing["relay_d0"] << [](pson& in){
        if(in.is_empty()){
            in = (bool) digitalRead(D0);
        }else{
            digitalWrite(D0, in ? HIGH : LOW);
            digitalWrite(LED_D0, in ? HIGH : LOW);
        }
    };

    thing["relay_d1"] << [](pson& in){
        if(in.is_empty()){
            in = (bool) digitalRead(D1);
        }else{
            digitalWrite(D1, in ? HIGH : LOW);
            digitalWrite(LED_D1, in ? HIGH : LOW);
        }
    };

    thing["relay_d2"] << [](pson& in){
        if(in.is_empty()){
            in = (bool) digitalRead(D2);
        }else{
            digitalWrite(D2, in ? HIGH : LOW);
            digitalWrite(LED_D2, in ? HIGH : LOW);
        }
    };

    thing["relay_d3"] << [](pson& in){
        if(in.is_empty()){
            in = (bool) digitalRead(D3);
        }else{
            digitalWrite(D3, in ? HIGH : LOW);
            digitalWrite(LED_D3, in ? HIGH : LOW);
        }
    };

    // example for controlling led
    thing["led"] << digitalPin(LED_BUILTIN);
    thing["led_r"] << digitalPin(LEDR);

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