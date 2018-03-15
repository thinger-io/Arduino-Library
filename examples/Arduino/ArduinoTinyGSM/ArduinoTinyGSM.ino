// Select your modem:
#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_A6
//#define TINY_GSM_MODEM_A7
//#define TINY_GSM_MODEM_M590

// uncomment line for debug
// #define _DEBUG_

// Can be installed from Library Manager or https://github.com/vshymanskyy/TinyGSM
#include <TinyGsmClient.h>
#include <ThingerTinyGSM.h>

// Emulate Serial1 on pins 10/11 if HW is not present (use interrupt pin in RX for better performance)
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(10, 11); // RX, TX
#endif

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

// use your own APN config
#define APN_NAME "your_apn_name"
#define APN_USER "your_apn_user"
#define APN_PSWD "your_apn_password"

// set your cad pin (optional)
#define CARD_PIN ""

ThingerTinyGSM thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL, Serial1);

void setup() {
  // uncomment line for debug
  // Serial.begin(115200);

  // Serial for AT commands (can be higher with HW Serial, or even lower in SW Serial)
  Serial1.begin(57600);

  // set APN (you can remove user and password from call if your apn does not require them)
  thing.setAPN(APN_NAME, APN_USER, APN_PSWD);

  // set PIN (optional)
  // thing.setPIN(CARD_PIN);

  // resource input example (i.e, controlling a digitalPin);
  pinMode(LED_BUILTIN, OUTPUT);
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}