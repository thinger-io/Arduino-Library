#define THINGER_SERIAL_DEBUG

// if the saved WiFi is not available (i.e., the device was moved), the config portal opens
// for 180 seconds after 3 failed connections, and then the device tries the saved WiFi again
// #define THINGER_WEBCONFIG_ATTEMPTS 3
// #define THINGER_WEBCONFIG_PORTAL_TIMEOUT 180

// Requires WifiManager from Library Manager or https://github.com/tzapu/WiFiManager
#include <ThingerESP8266WebConfig.h>

ThingerESP8266WebConfig thing;

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  /*
    Steps for getting the ESP8266 WebConfig working:
    1. Connect to Thinger-Device WiFi with your computer or phone, using thinger.io as WiFi password
    2. Wait for the configuration window, or navigate to http://192.168.4.1 if it does not appear
    3. Configure the wifi where the ESP8266 will be connected, and your thinger.io device credentials
    4. Your device should be now connected to the platform.
    More details at http://docs.thinger.io/arduino/
  */
}

void loop() {
  thing.handle();
}