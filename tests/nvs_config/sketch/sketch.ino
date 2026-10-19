// Compile-only API integration sketch, also suitable for a new test device.
// Remove SKIP_LEGACY_MIGRATION when updating a device with config.pson.
#define THINGER_ESP32_CONFIG_SKIP_LEGACY_MIGRATION
#include <ThingerESP32WebConfig.h>

ThingerESP32WebConfig device("user", "device", "credential");

void setup() {
    device.add_wifi("network-a", "password-a");
    device.add_wifi("network-b", "password-b");
    if(!device.wifi_storage_ok()) return;
    // On subsequent boots, both add_wifi calls above can be omitted.
    device.restore_wifi_networks();
    std::vector<std::string> ssids;
    if(!device.list_wifi(ssids)) return;
    device.remove_wifi("network-b");
    // Call on an explicit reset action, rather than on each normal boot.
    if(!device.clear_wifi()) return;
}

void loop() { device.handle(); }
