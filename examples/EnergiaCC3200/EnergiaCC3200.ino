#include <WiFi.h>
#include <ThingerWifi.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

#define SSID "your_wifi_ssid"
#define SSID_PASSWORD "your_wifi_ssid_password"

ThingerWifi thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // set the boards led to output
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"]["green"] << digitalPin(GREEN_LED);
  thing["led"]["yellow"] << digitalPin(YELLOW_LED);
  thing["led"]["red"] << digitalPin(RED_LED);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}