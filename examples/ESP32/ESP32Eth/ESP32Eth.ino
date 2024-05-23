// It may be required to define this according to your specific board
// Example RMII LAN8720 (Olimex, etc.)
#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE  ETH_PHY_LAN8720
#define ETH_PHY_ADDR  0
#define ETH_PHY_MDC   23
#define ETH_PHY_MDIO  18
#define ETH_PHY_POWER -1
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#endif

// enable debug output over serial
#define THINGER_SERIAL_DEBUG

// disable certificate validation
// #define THINGER_INSECURE_SSL

// define private server instance
// #define THINGER_SERVER "acme.aws.thinger.io"

#include <ThingerESP32Eth.h>
#include <ThingerESP32OTA.h>
#include "arduino_secrets.h"

// initialize thinger instance
ThingerESP32Eth thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Initialize ESP32OTA OTA
// use Thinger.io VSCode Studio extension + Platformio to upgrade the device remotely
ThingerESP32OTA ota(thing);

void setup() {

    // enable serial for debugging
    Serial.begin(115200);

    // example of fixed ip address (dhcp is used by default)
    //thing.set_address("192.168.1.55", "192.168.1.1", "255.255.255.0", "8.8.8.8", "8.8.4.4");

    // set desired hostname
    thing.set_hostname("ESP32Eth");

    // resource output example (i.e. reading a sensor value)
    thing["eth"] >> [](pson& out){
        out["hostname"] = ETH.getHostname();
        out["mac"] = ETH.macAddress();
        out["ip"] = ETH.localIP().toString();
        out["link"] = ETH.linkSpeed();
    };

    // more details at http://docs.thinger.io/arduino/
}

void loop() {
    thing.handle();
}