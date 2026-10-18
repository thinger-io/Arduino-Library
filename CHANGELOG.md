## 2.44.0

#### General

- **Added** `thing.get_state()`, which returns the last connection state reported to the state listener (i.e., `ThingerClient::THINGER_AUTHENTICATED` when the device is online) ([#52](https://github.com/thinger-io/Arduino-Library/issues/52)).
- **Fixed** `multiple definition of CA_ROOT_CERTIFICATE` when the library is included from several source files ([#41](https://github.com/thinger-io/Arduino-Library/issues/41)).

#### TinyGSM

- **Fixed** `ThingerTinyGSM` with `THINGER_FREE_RTOS` on ESP32: it now supports `thing.start()` to run in its own FreeRTOS task, as `ThingerESP32` ([#50](https://github.com/thinger-io/Arduino-Library/issues/50)).

## 2.43.2

#### Console

- **Fixed** crash on the second and following commands of `ThingerConsole` ([#55](https://github.com/thinger-io/Arduino-Library/issues/55)): the command parser used `static` lambdas capturing by reference, so they kept pointing to the variables of the first call. Note that `ThingerConsole` requires `std::function`, so it is not available on AVR boards.

## 2.43.1

#### WiFi

- **Added** an optional `ca_pem` parameter to `add_wifi_enterprise()` (ESP32): the CA certificate of the authentication server. With it, the device validates the server and does not send the credentials to a fake access point with the same SSID. Without it, the server is not validated (WiFiMulti does not support it). Only one enterprise network can use it, and the device tries the other networks if it cannot connect.
- **Changed** the identity of `add_wifi_enterprise()` defaults to the username when empty, as most authentication servers require an outer identity.

#### ESP8266 / Raspberry Pi Pico W

- **Fixed** `set_root_ca()` had no effect on ESP8266 and Pico W / Pico 2 W when called in `setup()`, as the certificate was loaded when constructing `thing`. Now it is loaded when connecting.

## 2.43.0

#### WiFi

- **Added** several WiFi networks on ESP32, ESP8266 and Raspberry Pi Pico W / Pico 2 W: call `add_wifi()` once per network, and the device connects to the strongest known one (also when it reconnects). With a single network it connects directly, as before, so hidden networks keep working.
- **Added** `add_wifi_enterprise(ssid, username, password, identity)` for WPA2-Enterprise networks (PEAP/EAP-MSCHAPv2, i.e., eduroam) on ESP32 with Arduino core 3.x.

#### General

- **Added** `THINGER_ROOT_CA` to define at compile time the CA root certificate (PEM) used to validate the server, i.e., for instances with their own CA. Define it before including the library; it takes precedence over the default certificates. `set_root_ca()` keeps changing it at runtime.

## 2.42.0

#### Raspberry Pi Pico W / Pico 2 W

- **Added** `ThingerPicoW` for Raspberry Pi Pico W and Pico 2 W (and other RP2040/RP2350 boards with CYW43 WiFi) using the [Arduino-Pico](https://github.com/earlephilhower/arduino-pico) core, with TLS certificate validation (BearSSL + NTP).
- **Added** `ThingerPicoOTA`: OTA updates stored in LittleFS and applied by the core bootloader. Requires a Flash Size option with a file system large enough for the firmware. Optional gzip compression with `THINGER_OTA_COMPRESSION`.
- **Added** device reboot on the Arduino-Pico core.

#### Arduino UNO Q

- **Added** `ThingerUnoQ` for the Arduino UNO Q: the sketch runs on the MCU (Arduino Zephyr core) and connects through the Linux side with the TCP client of [Arduino_RouterBridge](https://github.com/arduino-libraries/Arduino_RouterBridge). TLS is done on Linux (TLS 1.3).

#### Arduino UNO R4 WiFi

- **Added** `ThingerR4OTA` (experimental, not yet tested on hardware): OTA updates stored in the ESP32-S3 module, which programs the board on reboot. Requires the UNO R4 WiFi firmware 0.4.1 or later.

#### ESP32

- **Verified** ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C5, ESP32-C6 and ESP32-P4 (Ethernet) on ESP32 Arduino core 3.3, now compiled on every change.

## 2.41.1

#### General

- **Added** GitHub Actions workflow that compiles the examples for a matrix of boards on every push, pull request and weekly (also against the latest cores).
- **Fixed** `THINGER_USE_STATIC_MEMORY` had no effect: the library checked `THINGER_USE_STATIC__MEMORY` (double underscore), which is still accepted for backward compatibility. Define it as a build flag, as `ThingerClient.cpp` is compiled apart from the sketch.
- **Fixed** `invertedDigitalPin` was not defined outside AVR and ESP8266 (ESP32, SAMD, Mbed, Renesas, etc.), and reported the opposite state when read.
- **Fixed** RAM usage on AVR boards: the CA root certificate (about 2 KB) was copied to RAM even if AVR does not use TLS. Sketches like ArduinoYun or ESP8266AT on Arduino UNO fit again.

#### ESP32

- **Fixed** `ThingerESP32OTA` compilation with ESP32 Arduino core 3.3 (ESP-IDF 5.5), where `rom/miniz.h` moved to `miniz.h`. OTA compression keeps working on cores 2.x and 3.x.
- **Fixed** `ThingerESP32Eth` compilation with ESP32 Arduino core 3.x, where `ETH.h` no longer includes `WiFi.h`.
- **Fixed** `ESP32WebConfig` example on boards without `LED_BUILTIN`.
- **Fixed** missing `arduino_secrets.h` in the Thinger NB-IoT Core example.

#### TinyGSM

- **Fixed** `ThingerESP8266AT` compilation with TinyGSM 0.11 and 0.12, by using the concrete `TinyGsmClient` class.
- **Fixed** `ThingerTinyGSM` compilation on AVR boards, which do not provide `std::function` for `setModuleReset`.

#### ESP8266

- **Fixed** `SonoffTouch` example, updated to `ThingerESP8266WebConfig`.

## 2.41.0

#### OTA

- **Added** support for configurable OTA timeout. Devices can now receive and apply timeout values from the VSCode extension during OTA initialization, allowing per-device timeout configuration for operations over slow connections (GSM/GPRS).

## 2.40.0

#### NB-IOT CORE

- **Added** initial support for [Thinger NB-IOT Core](https://docs.thinger.io/others/hardware/thinger32-nb-iot). A module based on BC660 using Narrowband Technology.

## 2.31.0

#### ESP32

- **Fixed** compilation with newer Arduino Frameworks starting from version 3.0

#### Arduino R4

- **Added** initial support for Arduino R4 WiFi board. OTA support is not available yet.


#### OTA

- **Fixed** empty value for THINGER_OTA_VERSION when it is not defined.


## 2.30.0

#### OTA

- **Added** support for defining `THINGER_OTA_VERSION` to set the firmware version on the device. This build flag allows the VSCode extension (from version 1.1.0) to skip updates on devices with the same firmware version.
- **Improved** compressed/uncompressed checksum verification on OTA updates.

#### Arduino Portenta/Opta Devices

- **Added** OTA implementation for Arduino Portenta/Opta devices, including LZSS compression.
- **Initial support** for Arduino Portenta/Opta Ethernet connectivity.
- **Updated** Arduino Portenta examples for using thinger.io as an Mbed thread by using `thing.start()`.
- **Added** specific Arduino Opta examples, including relay and status LED control.

2.29.0

#### ESP32 Ethernet 
- Improve ESP32 Ethernet TLS support by using stock Arduino WiFiClientSecure
- Fix ESP32 log events related to Ethernet link status
- Update ESP32 Ethernet default example including OTA support
- Allow using THINGER_INSECURE_SSL definition to allow using self-signed certificates on ESP32 Ethernet

2.28.0

- Initial support for ESP32 Ethernet connection over TLS

2.27.0

- Fix compiling issues for AVR
- Avoid trying to send keep-alive/streams on the first connection timeout

2.26.0

- Added CMakeLists for integration with ESP-IDF as component
- Add ThingerClient methods to detect socket connection failures to improve network reconnection mechanisms
- Improve BC66
- Improve MKRNB1500

2.25.2

- Fix Compiling on WiFi devices without WiFi.begin() support after adding the WebConfig compatibility on ESP32.
- Fix PORTENTA H7 compiling issue: does not implement WiFi.begin(SSID).

2.25.1

- Fix ESP32 Webconfig issue
- Improve ESP8266 NTP sync

2.25.0

- Add support for ESP32 Webconfig
- Improve OTA by stopping user flows, handling timeouts, providing callbacks, etc.
