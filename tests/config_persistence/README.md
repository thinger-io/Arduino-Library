# Preserve WebConfig files and report configuration save failures

## Regression scenario

On ESP8266, save_configuration() opens /config.pson with mode w, ignores failed or partial encoder writes, and returns true whenever the file opened. Storage exhaustion can therefore truncate the old configuration and report success. The common WebConfig portal also ignores a false return from save_configuration(), so ESP32 NVS failures and other storage errors are hidden from the portal result.

## Run

From the repository root, using a C++11 compiler:

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/config_persistence/support -Isrc tests/config_persistence/main.cpp -o regression
./regression
~~~

On Windows use regression.exe. zig c++ (tested with Zig 0.13.0) may replace c++.
Do not define NDEBUG: assertions are the oracle. __arm__ selects a mocked reset
implementation; this is a host build, not a cross-compilation.

For a negative control, compile this same test against the unmodified 2.49.0
src directory (commit c6f62e01e80b5e7379fd7b41322586f295773a36).
The original version compiles but fails the regression assertions.

## Coverage

Filesystem mount/open failures; zero-byte and partial writes; rename failure; preservation of the original file; successful replacement and temporary-file cleanup; portal failure propagation and suppression of the configuration callback; successful retry.

## Scope and limitations

The common portal completion callback now reports false on persistence failure even if WiFi association succeeded. This does not roll back WiFi driver settings or credentials already copied into RAM. File::flush() and close() return void in the Arduino API, so this change does not claim to detect every low-level sync failure or to constitute a power-loss test. The storage fault-injection test uses a LittleFS double; physical flash and NVS exhaustion were not tested.

The tests include the production headers and small platform/transport doubles.
No physical board or real service account is required.
