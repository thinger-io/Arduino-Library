# Restore saved WebConfig parameters before every portal entry

## Regression scenario

When the Thinger configuration still exists but saved WiFi settings have been erased, connect_network() opens the initial provisioning path without restoring custom parameter values. The portal starts with constructor defaults and saving it can overwrite previously persisted application settings even if the user changes only WiFi. Restoration previously occurred only after repeated failures to connect to a saved network.

## Run

From the repository root, using a C++11 compiler:

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/webconfig_restore/support -Isrc tests/webconfig_restore/main.cpp -o regression
./regression
~~~

On Windows use regression.exe. zig c++ (tested with Zig 0.13.0) may replace c++.
Do not define NDEBUG: assertions are the oracle. __arm__ selects a mocked reset
implementation; this is a host build, not a cross-compilation.

For a negative control, compile this same test against the unmodified 2.49.0
src directory (commit c6f62e01e80b5e7379fd7b41322586f295773a36).
The original version compiles but fails the regression assertions.

## Coverage

Existing custom values are preserved with no saved WiFi settings and after the three-failed-connections fallback. A missing configuration and a non-string stored value retain the declared defaults.

## Scope and limitations

Tests use a WiFiManager double that accepts the displayed values without user edits. ESP32/ESP8266 compilation exercises the official WiFiManager 2.0.17 API; no browser/captive-portal interaction or physical radio test was performed.

The tests include the production headers and small platform/transport doubles.
No physical board or real service account is required.
