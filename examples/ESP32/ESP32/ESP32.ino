#define THINGER_SERIAL_DEBUG

#include <ThingerESP32.h>
#include "arduino_secrets.h"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(16, OUTPUT);

  thing.add_wifi(SSID, SSID_PASSWORD);
  // add more networks calling add_wifi again: it connects to the strongest one
  // thing.add_wifi("other_ssid", "other_password");
  // WPA2-Enterprise networks (PEAP), i.e., eduroam. Pass the CA certificate of the authentication
  // server (provided by your organization) as the last parameter to validate it
  // thing.add_wifi_enterprise("eduroam", "user@domain", "password", "anonymous@domain", EDUROAM_CA);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["GPIO_16"] << digitalPin(16);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}