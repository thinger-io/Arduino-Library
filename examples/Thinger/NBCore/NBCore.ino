
// uncomment to use custom iot server
//#define THINGER_SERVER "acme.thinger.io"

// uncomment to enable thinger debug messages
#define THINGER_SERIAL_DEBUG

// use only if using M2 based module
// #define THINGER_NB_M2_CORE

// Requires TinyGSM (https://github.com/vshymanskyy/TinyGSM)
#include <ThingerNBCore.h>
#include <ThingerESP32OTA.h>
#include "arduino_secrets.h"

ThingerNBCore thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerESP32OTA ota(thing, 512);

String iccid;

void setup(){
    Serial.begin(115200);

    // start modem
    while (!thing.init_modem()){
        delay(1000);
    }

    // read SIM ICCID (sim card ID)
    iccid = thing.get_sim_iccid();
    THINGER_DEBUG_VALUE("NB-IOT", "SIM ICCID: ", iccid.c_str());

    // set username and password with ICCID
    thing.set_credentials(USERNAME, iccid.c_str(), iccid.c_str());

    // sample device resources
    thing["voltage"] >> outputValue(thing.get_batt_voltage());
    
    thing["operator"] >> outputValue(thing.get_operator());

    thing["modem"] >> [](pson &out)
    {
        out["modem"] = thing.get_modem_info();
        out["IMEI"] = thing.get_modem_imei();
        out["CCID"] = thing.get_sim_iccid();
        out["operator"] = thing.get_operator();
    };
}

void loop(){
    // iotmp handle
    thing.handle();
}