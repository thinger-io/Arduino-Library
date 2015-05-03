#include <YunClient.h>
#include <ThingerYun.h>

ThingerYun yun("USERNAME", "DEVICE", "CREDENTIAL");
const int ledPin =  13;

void setup() {
  pinMode(ledPin, OUTPUT);
  Bridge.begin();
  yun["led"].in() = [](pson& in){ digitalWrite(ledPin, in ? HIGH : LOW); };
}

void loop() {
  yun.handle();
}
