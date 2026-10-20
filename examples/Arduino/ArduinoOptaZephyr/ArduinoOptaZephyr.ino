#define THINGER_SERIAL_DEBUG

// Arduino Opta on the Arduino Zephyr core (Arduino Zephyr Boards). Install the Zephyr loader once with
// Tools > Burn Bootloader. If the WiFi does not start, install the WiFi module firmware with the
// WiFiFirmwareUpdater example of the Arduino Mbed OS core (STM32H747_System library)
#include <ThingerZephyr.h>
#include "arduino_secrets.h"

// pins of the Opta on the Zephyr core: relays, their status leds, user button and inputs I1 to I8
#define RELAY_D0 D0
#define RELAY_D1 D1
#define RELAY_D2 D2
#define RELAY_D3 D3
#define LED_D0 D4
#define LED_D1 D5
#define LED_D2 D6
#define LED_D3 D7
#define BUTTON_USER D11

ThingerZephyr thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// voltage of an input (0-10V), from its 12-bit reading and the voltage divider of the Opta
float input_voltage(int pin) {
  return analogRead(pin) * (3.249 / 4095.0) / 0.3034;
}

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  // configure relays and their status leds for output
  pinMode(RELAY_D0, OUTPUT);
  pinMode(RELAY_D1, OUTPUT);
  pinMode(RELAY_D2, OUTPUT);
  pinMode(RELAY_D3, OUTPUT);
  pinMode(LED_D0, OUTPUT);
  pinMode(LED_D1, OUTPUT);
  pinMode(LED_D2, OUTPUT);
  pinMode(LED_D3, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_USER, INPUT);

  // read the inputs with 12 bits
  analogReadResolution(12);

  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  // example for controlling relays and status led
  thing["relay_d0"] << [](pson& in){
    if(in.is_empty()){
      in = (bool) digitalRead(RELAY_D0);
    }else{
      digitalWrite(RELAY_D0, in ? HIGH : LOW);
      digitalWrite(LED_D0, in ? HIGH : LOW);
    }
  };

  thing["relay_d1"] << [](pson& in){
    if(in.is_empty()){
      in = (bool) digitalRead(RELAY_D1);
    }else{
      digitalWrite(RELAY_D1, in ? HIGH : LOW);
      digitalWrite(LED_D1, in ? HIGH : LOW);
    }
  };

  thing["relay_d2"] << [](pson& in){
    if(in.is_empty()){
      in = (bool) digitalRead(RELAY_D2);
    }else{
      digitalWrite(RELAY_D2, in ? HIGH : LOW);
      digitalWrite(LED_D2, in ? HIGH : LOW);
    }
  };

  thing["relay_d3"] << [](pson& in){
    if(in.is_empty()){
      in = (bool) digitalRead(RELAY_D3);
    }else{
      digitalWrite(RELAY_D3, in ? HIGH : LOW);
      digitalWrite(LED_D3, in ? HIGH : LOW);
    }
  };

  // example for controlling led
  thing["led"] << digitalPin(LED_BUILTIN);

  // user button (pressed when low)
  thing["button"] >> outputValue(digitalRead(BUTTON_USER) == LOW);

  // voltage of the inputs I1 to I8
  thing["inputs"] >> [](pson& out){
    out["I1"] = input_voltage(A0);
    out["I2"] = input_voltage(A1);
    out["I3"] = input_voltage(A2);
    out["I4"] = input_voltage(A3);
    out["I5"] = input_voltage(A4);
    out["I6"] = input_voltage(A5);
    out["I7"] = input_voltage(A6);
    out["I8"] = input_voltage(A7);
  };

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}
