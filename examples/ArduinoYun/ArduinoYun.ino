#include <YunClient.h>
#include <ThingerYun.h>

ThingerYun yun("USERNAME", "DEVICE", "CREDENTIAL");

void setup() {
  pinMode(13, OUTPUT);
  Bridge.begin();
  yun["led"].in() = [](pson& in){ digitalWrite(13, in ? HIGH : LOW); };
}

void loop() {
  yun.handle();
}
