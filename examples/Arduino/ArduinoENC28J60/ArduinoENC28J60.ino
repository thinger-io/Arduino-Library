// UIPEthernet for ENC28J60: https://github.com/ntruchsess/arduino_uip
#include <ThingerENC28J60.h>

#define USERNAME "your_username"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

ThingerENC28J60 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // ENC28J60 using fixed IP Address. DHCP is too big for the sketch.
  uint8_t mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
  Ethernet.begin(mac, IPAddress(192, 168, 1, 125));

  pinMode(LED_BUILTIN, OUTPUT);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}