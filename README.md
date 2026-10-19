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

## ESP32 OTA compatibility metadata

Define `THINGER_OTA_CHECK(FIELD)` before including `ThingerESP32OTA.h` to prevent
accidental updates with firmware intended for a different PCB or connectivity mode:

```cpp
#define THINGER_OTA_CHECK(FIELD) \
    FIELD("PCB_MODEL", "SENSOR_CURRENT_GPIO_ANALOG") \
    FIELD("PCB_VERSION", "V2") \
    FIELD("COMMUNICATION_MODE", "WIFI")

#include <ThingerESP32.h>
#include <ThingerESP32OTA.h>

ThingerESP32 thing("user", "device", "credential");
ThingerESP32OTA ota(thing);
```

This single definition produces both the `THINGER_OTA_CHECK` output resource and a
versioned manifest inside the application binary. The updated Thinger.io VS Code
extension compares the running device's resource with the exact image it will send,
shows changed, added and removed fields, and requires **Yes, continue** before an
incompatible update can begin. **No**, closing the dialog or cancelling the upload
sends no firmware. Each device in a product is checked separately.

Names are user-defined, case-sensitive UTF-8 strings. Keys and values must be C++
string literals (or macros expanding to literals), with 1–32 unique fields, keys of
1–64 bytes, values of 0–256 bytes, and at most 4096 bytes for the packed payload.
Embedded NUL bytes are rejected at compile time. Do not use the reserved
`@THINGER_OTA_CHECK:` marker in field text. Use the same declaration in every source
file that includes the OTA header, preferably through a shared configuration header.

Do not redefine the automatically registered resource with a separate callback:
that would break the single source of truth. Omitting the macro preserves the
existing ESP32 firmware behavior; the updated extension requests explicit
confirmation when either side has no metadata. Invalid metadata or failed resource
requests stop the update. The resource must be readable by the extension's access
token. Metadata is public compatibility information: do not include passwords or
device credentials. It describes the intended hardware; it does not detect the PCB
or authenticate the image, and a user can explicitly override a mismatch.

See [ESP32OTACheck](examples/ESP32/ESP32OTACheck/ESP32OTACheck.ino). To compile both
example variants from this checkout, run `pio run -d tests/esp32-ota-check`.
`python tests/esp32-ota-check/validate.py --compiler <xtensa-esp32-elf-g++>` checks
accepted declarations and expected compilation failures without hardware.

The version 1 binary format is the ASCII header `@THINGER_OTA_CHECK:1@` plus NUL,
UTF-8 key/NUL/value/NUL pairs, an empty key (NUL), and the footer
`@END_THINGER_OTA_CHECK@` plus NUL. The array is part of the linked application image,
not appended after signing or checksum generation. Version and compatibility are
independent; existing `THINGER_OTA_VERSION` definitions remain supported.

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

## OTA updates

The boards with an OTA header can be updated remotely over the Internet. The [Thinger.io extension for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=thinger-io.thinger-io) builds the firmware with PlatformIO, sends it to the device (compressed when the board supports it) and reboots it.

![OTA with Visual Studio Code](https://s3.eu-west-1.amazonaws.com/thinger.io.files/vscode/iot-ota.gif)

## Documentation

Full documentation of the library, the boards and the platform: [docs.thinger.io](https://docs.thinger.io/devices/arduino).

Changes of every version: [CHANGELOG.md](CHANGELOG.md).

## License

MIT License. Copyright &copy; [Thinger.io](https://thinger.io). See [LICENSE](LICENSE).
