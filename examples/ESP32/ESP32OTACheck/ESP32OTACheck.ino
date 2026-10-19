// Define compatibility once, before the OTA header is included.
#ifndef OTA_CHECK_I2C
#define OTA_CHECK_I2C 0
#endif

#if OTA_CHECK_I2C
#define THINGER_OTA_CHECK(FIELD) \
    FIELD("PCB_MODEL", "SENSOR_CURRENT_I2C") \
    FIELD("PCB_VERSION", "V4") \
    FIELD("COMMUNICATION_MODE", "WIFI")
#else
#define THINGER_OTA_CHECK(FIELD) \
    FIELD("PCB_MODEL", "SENSOR_CURRENT_GPIO_ANALOG") \
    FIELD("PCB_VERSION", "V2") \
    FIELD("COMMUNICATION_MODE", "WIFI")
#endif

// Deliberately identical in both builds: version is independent of compatibility.
#define THINGER_OTA_VERSION ota_check_example
#include <ThingerESP32.h>
#include <ThingerESP32OTA.h>

ThingerESP32 thing("your_user", "your_device", "your_device_credential");
ThingerESP32OTA ota(thing);

void setup() {
    thing.add_wifi("your_ssid", "your_wifi_password");
    // THINGER_OTA_CHECK is registered automatically by the OTA constructor.
}

void loop() {
    thing.handle();
}
