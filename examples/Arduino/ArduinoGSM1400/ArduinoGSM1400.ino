#include <ThingerMKRGSM.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

#define PIN_NUMBER "your_pin"

#define GPRS_APN "your_apn_name"
#define GPRS_LOGIN "your_gprs_login"
#define GPRS_PASSWORD "your_gprs_password"

ThingerMKRGSM thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  Serial.begin(115200);

  // optional set pin number
  thing.set_pin(PIN_NUMBER);

  // set APN
  thing.set_apn(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD);

  // set builtin led to output
  pinMode(LED_BUILTIN, OUTPUT);

  // pin control example over internet (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}