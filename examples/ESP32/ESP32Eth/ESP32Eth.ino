// It may be required to define this according to your specific board
// This example works for Olimex ESP32-PoE-ISO

//#define ETH_PHY_ADDR 0
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
//#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_POWER 12
//#define ETH_PHY_MDC 23
//#define ETH_PHY_MDIO 18
//#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN

// enable debug output over serial
#define THINGER_SERIAL_DEBUG 

#include <ThingerESP32Eth.h>
#include "arduino_secrets.h"

ThingerESP32Eth thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

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