#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <ccspi.h>
#include <ThingerCC3000.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

#define SSID "your_wifi_ssid"
#define SSID_PASSWORD "your_wifi_ssid_password"

ThingerCC3000 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    // configure wifi network
    thing.add_wifi(SSID, SSID_PASSWORD);

    // resource input example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
    thing["led"] << [](pson& in){ bool on = in; };

    // resource output example (i.e. reading a sensor value)
    thing["millis"] >> [](pson& out){ out = millis(); };

    // resource input/output example (i.e. passing input values and generate a result)
    thing["in_out"] = [](pson& in, pson& out){
        out["sum"] = (long)in["value1"] + (long)in["value2"];
        out["mult"] = (long)in["value1"] * (long)in["value2"];
    };
}

void loop() {
  thing.handle();
}