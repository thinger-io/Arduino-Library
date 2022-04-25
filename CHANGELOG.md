2.25.2

- Fix Compiling on WiFi devices without WiFi.begin() support after adding the WebConfig compatibility on ESP32.
- Fix PORTENTA H7 compiling issue: does not implement WiFi.begin(SSID).

2.25.1

- Fix ESP32 Webconfig issue
- Improve ESP8266 NTP sync

2.25.0

- Add support for ESP32 Webconfig
- Improve OTA by stopping user flows, handling timeouts, providing callbacks, etc.