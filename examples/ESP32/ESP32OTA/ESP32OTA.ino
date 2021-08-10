#include <ThingerESP32.h>
#include <ThingerESP32OTA.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

#define SSID "your_wifi_ssid"
#define SSID_PASSWORD "your_wifi_ssid_password"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerESP32OTA ota(thing);

void setup() {
  pinMode(16, OUTPUT);

  // example of fixed ip address (by default it uses dhcp)
  ETH.config("192.168.1.55", "192.168.1.1", "255.255.255.0", "8.8.8.8", "8.8.4.4");

  thing.add_wifi(SSID, SSID_PASSWORD);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["GPIO_16"] << digitalPin(16);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}