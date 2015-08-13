#include <YunClient.h>
#include <ThingerYun.h>

#define USERNAME "your_user_name"
#define DEVICE_ID "your_device_id"
#define DEVICE_CREDENTIAL "your_device_credential"

ThingerYun thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
const int ledPin =  13;

void setup() {
    pinMode(ledPin, OUTPUT);

    // initialize bridge
    Bridge.begin();

    // resource input example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
    thing["led"] << [](pson& in){ digitalWrite(ledPin, in ? HIGH : LOW); };

    // resource output example (i.e. reading a sensor value)
    thing["millis"] >> [](pson& out){ out = millis(); };

    // resource input/output example (i.e. passing input values and do some calculations)
    thing["in_out"] = [](pson& in, pson& out){
        out["sum"] = (long)in["value1"] + (long)in["value2"];
        out["mult"] = (long)in["value1"] * (long)in["value2"];
    };
}

void loop() {
    thing.handle();
}
