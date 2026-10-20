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
| Arduino UNO R4 WiFi | `ThingerR4WiFi.h` | WiFi | `ThingerR4OTA.h` |
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
