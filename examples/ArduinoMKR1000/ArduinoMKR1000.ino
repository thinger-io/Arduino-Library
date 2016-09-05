#include <WiFi101.h>
#include <ThingerWifi101.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

#define SSID "your_wifi_ssid"
#define SSID_PASSWORD "your_wifi_ssid_password"
#define LED_PIN 6

ThingerWifi101 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  pinMode(LED_PIN, OUTPUT);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_PIN);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}