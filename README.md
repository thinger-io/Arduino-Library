# Thinger.io Arduino Library

[![Compile examples](https://github.com/thinger-io/Arduino-Library/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/thinger-io/Arduino-Library/actions/workflows/compile-examples.yml)
[![Release](https://img.shields.io/github/v/release/thinger-io/Arduino-Library)](https://github.com/thinger-io/Arduino-Library/releases)
[![Arduino Library Manager](https://www.ardu-badge.com/badge/thinger.io.svg?)](https://www.ardu-badge.com/thinger.io)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/thinger-io/library/thinger.io.svg)](https://registry.platformio.org/libraries/thinger-io/thinger.io)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Connect your Arduino devices to the [Thinger.io](https://thinger.io "Thinger.io IoT Cloud Platform") IoT platform with the IOTMP protocol. Define the resources of your device (sensors to read, outputs to switch, data to stream) in a few lines, and use them from the console, dashboards, the REST API or buckets. Over TLS, with remote OTA updates on most boards.

## Installation

- **Arduino IDE**: search for `thinger.io` in the Library Manager.
- **arduino-cli**: `arduino-cli lib install thinger.io`
- **PlatformIO**: add `thinger-io/thinger.io` to `lib_deps` in `platformio.ini`.

## Quick start

```cpp
#define THINGER_SERIAL_DEBUG

#include <ThingerESP32.h>

#define LED_PIN 2 // built-in LED of most ESP32 DevKit boards

ThingerESP32 thing("username", "deviceId", "deviceCredential");

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // call add_wifi() again to add more networks: it connects to the strongest one
  thing.add_wifi("wifi_ssid", "wifi_password");

  // control the LED from the platform
  thing["led"] << digitalPin(LED_PIN);

  // read a value from the platform
  thing["millis"] >> outputValue(millis());
}

void loop() {
  thing.handle();
}
```

Every board has its own example in [`examples/`](examples), also available in the Arduino IDE under *File > Examples > thinger.io*.

## Supported boards

Each board family uses its own header. Every board in this table is compiled on every change by the [CI](.github/workflows/compile-examples.yml), with the pinned cores and every week with the latest ones.

| Boards | Header | Connectivity | OTA |
|---|---|---|---|
| ESP32, ESP32-S2, S3, C3, C5, C6, Arduino Nano ESP32, Seeed XIAO ESP32 | `ThingerESP32.h` | WiFi | `ThingerESP32OTA.h` |
| ESP32 with Ethernet, ESP32-P4 | `ThingerESP32Eth.h` | Ethernet | `ThingerESP32OTA.h` |
| ESP8266 | `ThingerESP8266.h` | WiFi | `ThingerESP8266OTA.h` |
| Raspberry Pi Pico W, Pico 2 W ([Arduino-Pico](https://github.com/earlephilhower/arduino-pico) core) | `ThingerPicoW.h` | WiFi | `ThingerPicoOTA.h` |
| Arduino UNO R4 WiFi | `ThingerR4WiFi.h` | WiFi | `ThingerR4OTA.h` (experimental) |
| Arduino UNO Q | `ThingerUnoQ.h` | Through the Linux side ([Arduino_RouterBridge](https://github.com/arduino-libraries/Arduino_RouterBridge)) | |
| Arduino Nano 33 IoT, MKR WiFi 1010 | `ThingerWiFiNINA.h` | WiFi | `ThingerWiFiNINAOTA.h` |
| Arduino Nano RP2040 Connect | `ThingerMbed.h` | WiFi | `ThingerMbedOTA.h` |
| Arduino Portenta H7, Opta | `ThingerMbed.h`, `ThingerMbedEth.h` | WiFi, Ethernet | `ThingerPortentaOTA.h` |
| Arduino MKR NB 1500 | `ThingerMKRNB.h` | NB-IoT, LTE-M | `ThingerMKRNBOTA.h` |
| Arduino MKR GSM 1400 | `ThingerMKRGSM.h` | 2G, 3G | |
| Arduino MKR1000 | `ThingerWifi101.h` | WiFi | |
| Thinger NB-IoT Core (BC660) | `ThingerNBCore.h` | NB-IoT | `ThingerESP32OTA.h` |
| Any board with a cellular modem supported by [TinyGSM](https://github.com/vshymanskyy/TinyGSM) | `ThingerTinyGSM.h` | 2G, LTE-M, NB-IoT | |
| Any board with an ESP8266 as WiFi modem (AT commands) | `ThingerESP8266AT.h` | WiFi | |
| Arduino with Ethernet shield (W5100/W5500) | `ThingerEthernet.h` | Ethernet | |
| Arduino with ENC28J60 | `ThingerENC28J60.h` | Ethernet | |
| Arduino Yún | `ThingerYun.h` | WiFi, Ethernet | |

**Legacy** (discontinued hardware, kept for compatibility): Adafruit CC3000 (`ThingerCC3000.h`) and the Arduino WiFi shield (`ThingerWifi.h`), still compiled on CI; Texas Instruments CC3200 with Energia and LinkIt ONE (`ThingerLinkItOneWifi.h`, `ThingerLinkItOneGPRS.h`), whose cores are no longer available.

## ESP32 WebConfig storage and migration

`ThingerESP32WebConfig` stores the entire configuration object in the ESP32's
default NVS partition, namespace `thinger`, key `config`. The versioned blob
contains the original PSON payload, including custom and unknown fields. Later
portal saves update the loaded object rather than dropping unregistered fields.
WiFiManager's native Wi-Fi credentials remain managed by the ESP32 Wi-Fi core
in NVS; they are not part of the upstream `/config.pson` schema. Any Wi-Fi fields
added to that file by an application are preserved with all other fields; the
application's configuration callback continues to interpret them.

When the NVS record is absent, the first load imports `/config.pson` from SPIFFS.
For ESP32 firmware that previously used LittleFS, define
`THINGER_ESP32_CONFIG_USE_LITTLEFS` before including `ThingerESP32WebConfig.h`.
Choose the filesystem actually used by the deployed firmware. The importer
never formats flash. It validates the complete PSON object, commits it to NVS,
checks the saved bytes and reloads from NVS before removing the legacy file.
Once a record exists, loading never consults the filesystem again, including
when legacy cleanup fails. A reset during import leaves either the original
file for retry or an authoritative NVS record.

NVS failures, invalid/unsupported records and an unreadable legacy volume
stop configuration loading rather than overwriting data through the portal.
The file remains untouched if import fails. The default maximum PSON payload is
16 KiB; override `THINGER_NVS_CONFIG_MAX_SIZE` before including the header for
larger deployments, subject to available RAM and NVS capacity.

For **new devices without a legacy filesystem**, define
`THINGER_ESP32_CONFIG_SKIP_LEGACY_MIGRATION` before including the header. This
allows first-time portal setup without mounting a filesystem; do not enable it
on a deployed fleet that needs migration. Preserve the old filesystem partition
and NVS partition during the firmware update until migration has completed.

`clean_credentials()` commits an empty-state NVS record instead of removing
the key. This prevents a leftover legacy file from restoring cleared settings.
It preserves the existing method's scope: it does not erase WiFiManager's
separate native Wi-Fi configuration. Explicit cleanup is also available to an
application that intentionally discards an unreadable configuration.

On **all ESP32 Wi-Fi clients**, including `ThingerESP32` and WebConfig,
`add_wifi(ssid, password)` now commits the complete personal-network list to the
separate NVS key `thinger/wifi`. Calls for the same SSID update its password;
identical calls do not rewrite flash. Null and empty passwords both represent
an open network. The library owns copies of the credential strings.

Initialization is lazy, so global constructors do not access NVS. `add_wifi()`
loads and merges the existing list before updating it. Connection attempts load
the list and rebuild `WiFiMulti` even if the sketch never calls `add_wifi()`
after reboot. WebConfig checks this restored list before opening its portal,
and successful portal submissions are added to the same persistent list.
An existing ESP32 native Wi-Fi configuration remains the fallback only while
the `thinger/wifi` record has never been created. An explicitly empty saved list
does not enable that fallback.

The `add_wifi()` signature remains `void`. Check `wifi_storage_ok()` immediately
after a call to detect storage errors or rejected credentials. The accepted
limits match WiFiMulti in the tested cores: 1–31 bytes for SSID, 0–63 bytes for
password, at most 255 list entries, and the configured NVS payload limit.
`restore_wifi_networks()` is also available to initialize or retry loading
explicitly. A read error or invalid Wi-Fi record blocks connecting with that
record; it does not replace the saved list with an empty one. A failed update
does not register the proposed credentials in RAM. After a commit whose
verification failed, the next load may recover the complete new record.

The ESP32 personal-network registry also provides:

```cpp
std::vector<std::string> ssids;
if (thing.list_wifi(ssids)) {
    for (const auto& ssid : ssids) Serial.println(ssid.c_str());
}

if (!thing.remove_wifi("network-b") && !thing.wifi_storage_ok()) {
    // Handle a storage/restore failure. An absent SSID also returns false,
    // but leaves wifi_storage_ok() true after a successful load.
}
```

`list_wifi(ssids)` loads the saved list lazily and returns an independent
snapshot of SSIDs, in registration order. It never exposes passwords or writes
flash. A valid empty list returns true; on failure the output stays unchanged.
`remove_wifi(ssid)` matches the SSID exactly, including case. It commits the
remaining complete list and rebuilds WiFiMulti before reporting success. If the
removed SSID is the driver's current network, it requests a disconnect and
clearing that driver's saved AP. Removing an absent/invalid SSID does not write
flash. A failed update may have committed before verification failed, as with
`add_wifi()`; retry loading before deciding which list survived.

Removing the last personal network leaves a valid empty NVS record. A reboot
cannot resurrect an old AP through the library's native-driver fallback.
WebConfig opens the portal when another network is required; other clients
report `NETWORK_NOT_CONFIGURED`. Listing/deletion only applies to networks
registered by `add_wifi()` or the portal, preserving RAM enterprise networks
and the separate PSON configuration. Future explicit `add_wifi()` calls for a
removed SSID register it again, including unconditional calls in `setup()`.

`bool clear_wifi()` removes all personal networks registered by `add_wifi()` or
the portal, commits a valid empty list to NVS and rebuilds WiFiMulti. For example:

```cpp
if (!thing.clear_wifi()) {
    // Handle the storage failure; the clear could not be verified.
}
```

The empty record remains authoritative across resets, blocking native-driver
fallback even if old driver credentials survived separately. A current
non-enterprise connection is asked to disconnect and clear its saved AP.
PSON configuration, Thinger credentials, custom parameters, unrelated NVS keys,
and RAM-only enterprise credentials are preserved. An active enterprise
connection is retained. Explicit clearing can replace a malformed Wi-Fi record
without loading it first; it never formats or erases the NVS partition.
The return value is true only after commit, readback verification and rebuilding
the runtime list. As with other updates, a failed readback can follow a successful
commit; reload to determine the surviving state. New explicit `add_wifi()` calls
or a portal submission can register networks again after clearing.

`add_wifi_enterprise()` retains its existing RAM-only behavior. Adding/updating
a personal network preserves enterprise entries already registered in RAM.
`clean_credentials()` clears only the PSON configuration, preserving the Wi-Fi
list, just as it previously preserved the native Wi-Fi credentials. Stopping
calls to `add_wifi()` does not delete networks that were already persisted.
Opaque application-defined Wi-Fi fields in the migrated PSON are preserved;
their existing application callback still defines how to interpret them.

ESP8266 continues to use its existing LittleFS backend and RAM WiFiMulti list;
ESP32 NVS is not an ESP8266 storage API. Host tests in `tests/nvs_config` run in
CI against the real ThingerWifi, ESP32 WebConfig and PSON code and unmodified
WiFiMulti sources from ESP32 cores 2.0.17 and 3.3.12. Only radio, transport, NVS
and filesystem operations are simulated. The matrix covers SPIFFS, LittleFS
and new devices without a filesystem. Tests cover reboot without `add_wifi()`,
strongest-network selection, fallback, open/hidden networks, password updates,
duplicate prevention, portal persistence, failed writes/commits/reads, and full
PSON migration alongside multiple networks. They also verify independent SSID
snapshots, persistent deletion, active/last-network removal, blocking stale
native-driver fallback, deletion failure recovery, clearing all networks,
preservation of other NVS data, and clearing malformed records. Core fixture sources retain their
LGPL licenses. `tests/nvs_config/sketch` checks the public persistence API in the
ESP32 and Nano ESP32 board compilation matrix, alongside the existing examples.

## OTA updates

The boards with an OTA header can be updated remotely over the Internet. The [Thinger.io extension for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=thinger-io.thinger-io) builds the firmware with PlatformIO, sends it to the device (compressed when the board supports it) and reboots it.

![OTA with Visual Studio Code](https://s3.eu-west-1.amazonaws.com/thinger.io.files/vscode/iot-ota.gif)

## Documentation

Full documentation of the library, the boards and the platform: [docs.thinger.io](https://docs.thinger.io/devices/arduino).

Changes of every version: [CHANGELOG.md](CHANGELOG.md).

## License

MIT License. Copyright &copy; [Thinger.io](https://thinger.io). See [LICENSE](LICENSE).
