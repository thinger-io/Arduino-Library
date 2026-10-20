## 2.49.0

#### Arduino Zephyr core

- **Added** `ThingerZephyr` for boards with WiFi on the Arduino Zephyr core (Arduino Zephyr Boards), which replaces the Mbed OS core (end of life in July 2026): tested on Portenta H7 and Opta, with TLS certificate validation, and the non-blocking WiFi connection of 2.48.0. Example: `ArduinoZephyr`.
  - It uses its own TCP/TLS client on the Zephyr sockets, as the TLS client of the core (`ZephyrSSLClient` 1.0.0) cannot connect: it sets a list of credential tags with uninitialized entries, and registers the credentials again on every connection.
  - Install the Zephyr loader once with Tools > Burn Bootloader. If the WiFi does not start (`WiFi.status()` is `WL_NO_SHIELD`), install the WiFi module firmware with the `WiFiFirmwareUpdater` example of the Arduino Mbed OS core, as the Zephyr core reads it from the QSPI flash.
  - OTA updates will be available once the Arduino Zephyr core publishes its OTA support (already in development), as the current version (1.0.0) does not include it.

## 2.48.0

#### General

- **Changed** `thing.handle()` no longer blocks the `loop()` while the device is offline, so the sketch keeps running (i.e., to show the connection state on a display with `thing.get_state()`):
  - It does not wait to retry a failed connection (it used to wait 5 seconds with `delay()`).
  - WiFi boards connect to the network step by step, checking the connection on every `thing.handle()`, instead of waiting up to 30 seconds for the WiFi and 30 more for the IP address (and up to 50 seconds more inside `WiFi.begin()` with WiFiNINA, or 10 with the UNO R4 WiFi). The state is `NETWORK_CONNECTING` meanwhile. Connecting to several networks (`WiFiMulti`) still blocks a few seconds while scanning, and WebConfig still waits for the connection.
  - The TLS connection and the authentication with the server still block the `loop()` for a few seconds on every attempt, as cellular modems while connecting to the network.
- **Added** exponential backoff when the server cannot be reached or rejects the device (socket, TLS or authentication failures): the wait starts at `RECONNECTION_TIMEOUT` (5 seconds), and doubles on every failure up to `THINGER_RECONNECTION_MAX_TIMEOUT` (30 seconds), with a random jitter of up to 25%, so many devices do not reconnect at the same time (i.e., after a server restart). It is reset once the device connects. Network failures (i.e., WiFi or cellular) are still retried every `RECONNECTION_TIMEOUT`.

#### ESP32

- **Added** automatic rollback of OTA updates with `#define THINGER_OTA_ROLLBACK`: a new firmware must connect to Thinger.io within `THINGER_OTA_ROLLBACK_TIMEOUT` seconds (300 by default), or the device rolls back to the previous firmware, also if it reboots before (i.e., because of a crash). So a firmware with a bug, or with wrong WiFi or Thinger.io credentials, does not leave a remote device offline. It uses the rollback support of the ESP32 bootloader, and works on every ESP32 family (tested on an ESP32-C3).

#### WiFiNINA (Nano 33 IoT, MKR WiFi 1010)

- **Fixed** OTA updates leaving the device unable to boot when the firmware stored in the WiFi module is corrupted: the bootloader (SNU) programs it as it is on the next boot. Now the update reads the stored file back and verifies its MD5 checksum before applying it, and fails with `The firmware stored in the WiFi module is corrupted` otherwise, so the device keeps running its current firmware. The WiFi module does not report write errors ([WiFiNINA#242](https://github.com/arduino-libraries/WiFiNINA/issues/242)), and it returns up to 255 bytes per read ([WiFiNINA#333](https://github.com/arduino-libraries/WiFiNINA/issues/333)).
  Tested on a MKR WiFi 1010 (NINA firmware 1.4.8), where updates work. On one Arduino Nano 33 IoT unit, with the same NINA firmware (and also 3.0.1), the stored firmware was corrupted during the update, which left it unable to boot before this fix, and now fails safely.

#### Arduino Opta

- **Fixed** `ArduinoOptaWiFi` example not configuring the WiFi network with `thing.add_wifi()`. Tested on hardware: connection with TLS and OTA updates.

## 2.47.1

#### Arduino Nano RP2040 Connect

- **Fixed** OTA updates reported as successful, but the device rebooting with its previous firmware, when the new firmware does not include `ThingerMbedOTA.h` ([#46](https://github.com/thinger-io/Arduino-Library/issues/46)). The second stage bootloader (SFU) that applies the update is linked by `ThingerMbedOTA.h`, and it ignores a firmware without it, so the device could not be updated anymore. Now the update fails with `The firmware does not include ThingerMbedOTA.h, so it cannot be applied`. Tested on hardware with the Arduino Mbed OS Nano Boards core 4.6.0.

## 2.47.0

#### OTA

- **Added** OTA compatibility check, to avoid installing a firmware built for another hardware (i.e., another PCB version, or a cellular modem instead of WiFi), based on the proposal of @swinp ([#60](https://github.com/thinger-io/Arduino-Library/pull/60), [vscode-extension#7](https://github.com/thinger-io/vscode-extension/issues/7)). Declare the compatibility fields before including the OTA header:
  ```cpp
  #define THINGER_OTA_CHECK(FIELD) FIELD("PCB_MODEL", "SENSOR_I2C") FIELD("PCB_VERSION", "V2")
  ```
  The fields are embedded in the firmware binary, and reported by the device in the `$ota/options` resource, so the VS Code extension (1.2.0 or later) compares the device with the new firmware and asks for confirmation if they differ. The device also checks them, so it rejects an update from any client whose fields are different or missing, unless it is forced (i.e., confirmed by the user). Devices without fields are updated as before. Available on every board with OTA updates. Up to 16 fields, with keys of up to 32 characters and values of up to 128 characters, checked at compile time.

## 2.46.1

#### UNO R4 WiFi

- **Fixed** the CA root certificate was not sent to the WiFi module, so the TLS connection was validated with its own CA bundle, and servers with a private CA or `set_root_ca()` did not work. Now it is validated with the library CA certificate, as on the ESP32.
- **Fixed** build error with `THINGER_INSECURE_SSL`. The WiFi module cannot skip the certificate validation, so it uses its own CA bundle instead.
- **Changed** OTA updates (`ThingerR4OTA`) are no longer experimental: tested on hardware with the WiFi module firmware 0.6.0. Note that old WiFi module firmwares (i.e., 0.2.1) cannot connect with TLS to Thinger.io, so update it with Tools > Firmware Updater in the Arduino IDE.

## 2.46.0

#### WebConfig

- **Changed** `ThingerESP32WebConfig` stores the configuration (Thinger.io credentials and custom parameters) in NVS instead of SPIFFS ([#58](https://github.com/thinger-io/Arduino-Library/issues/58)). NVS keeps it on power loss, and it does not depend on the partition scheme, so it is not lost after flashing a firmware with another partition scheme. The configuration of previous versions is migrated from SPIFFS to NVS on the first boot, and the SPIFFS partition is no longer formatted if it cannot be mounted. Note that the WiFi credentials were already stored in NVS by the ESP32 WiFi driver.
- **Fixed** `ThingerESP32WebConfig` opening the config portal at boot, randomly, with a WiFi network already configured, so it looked like the WiFi credentials were lost ([#58](https://github.com/thinger-io/Arduino-Library/issues/58)). WiFiManager reads the stored network from the WiFi driver, that was not initialized yet.
- **Added** the config portal opens again when the device cannot connect to the saved WiFi network, i.e., it was moved to another place ([#49](https://github.com/thinger-io/Arduino-Library/issues/49)). After `THINGER_WEBCONFIG_ATTEMPTS` failed connections (3 by default, 0 to disable it), the portal is open for `THINGER_WEBCONFIG_PORTAL_TIMEOUT` seconds (180 by default), and then the device tries the saved network again. The portal shows the current values of the custom parameters, and the configuration does not change if the portal times out.
- **Fixed** the config portal opening at boot when the WiFi is configured and the credentials are set in the constructor, but there is no stored configuration (i.e., after erasing the file system).

## 2.45.0

#### General

- **Added** `thing.get_error()`, the reason of the last `NETWORK_CONNECT_ERROR`, or `ThingerClient::ERROR_NONE` once the network is connected ([#53](https://github.com/thinger-io/Arduino-Library/issues/53)). It can be read from the state listener or anytime:
  - WiFi (ESP32, ESP8266, Pico W and other WiFi boards): `NETWORK_NOT_CONFIGURED`, `NETWORK_NOT_FOUND`, `NETWORK_AUTH_ERROR` (usually a wrong password), `NETWORK_NO_IP`. WebConfig reports `NETWORK_NOT_CONFIGURED` if the config portal ends without configuring the WiFi.
  - Cellular (TinyGSM, NB-IoT Core, BC66, MKR NB 1500, MKR GSM 1400): `NETWORK_NOT_CONFIGURED` (no APN), `MODEM_NOT_RESPONDING`, `MODEM_SIM_ERROR`, `NETWORK_NOT_REGISTERED` (i.e., no coverage), `NETWORK_REGISTRATION_DENIED`, `NETWORK_APN_ERROR`, depending on what each modem reports.
  - Other failures, or clients that don't report the reason: `ERROR_UNKNOWN`.
- **Changed** the connection state (`get_state()`) and error (`get_error()`) are atomic on ESP32, so they can be read from another task or core (i.e., with `THINGER_FREE_RTOS`) without `thing.lock()`.

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
