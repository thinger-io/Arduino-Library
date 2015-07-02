#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ThingerWifi.h>

ThingerWifi esp8266("USERNAME", "DEVICE", "CREDENTIAL");

void setup() {
  esp8266.add_wifi("SSID", "SSID_PASSWORD");
  esp8266["sum"] = [](pson& in, pson& out){
    out["result"] = (int)in["value1"] + (int)in["value2"];
  };
}

void loop() {
  esp8266.handle();
}