# WiFi credential ownership regression

These host tests include the production `ThingerWifi.h` and protocol/client
headers. Small Arduino, transport and WiFi mocks capture the values passed to
the network API. No board, real network, or Thinger account is required.

From the repository root, compile and run with a C++11 compiler:

```sh
c++ -std=c++11 -pthread -D__arm__ -Itests/wifi_credentials/support -Isrc tests/wifi_credentials/main.cpp -o wifi-single
./wifi-single

c++ -std=c++11 -pthread -D__arm__ -DESP32 -Itests/wifi_credentials/support -Isrc tests/wifi_credentials/main.cpp -o wifi-multi
./wifi-multi

c++ -std=c++11 -pthread -D__arm__ -DESP32 -DCONFIG_ESP_WIFI_ENTERPRISE_SUPPORT=1 -Itests/wifi_credentials/support -Isrc tests/wifi_credentials/main.cpp -o wifi-enterprise
./wifi-enterprise
./wifi-enterprise enterprise
```

On Windows, the executable names end in `.exe`. `zig c++` (tested with Zig
0.13.0 on Windows) can replace `c++`. Do not define `NDEBUG`: assertions are
the test oracle. `__arm__` selects the stubbed reset implementation for the
generic host build; it does not cross-compile the executable.

Coverage:

- Modifying the original character arrays does not change registered values.
- Destroying or reallocating caller strings does not invalidate later connections.
- Reconnection, null passwords for open networks, and empty passwords work.
- Generic single-network replacement is safe, including input aliases to the
  client's existing credentials and clearing the registration with null fields.
- Failed allocation preserves the previous registration or leaves an empty client
  empty; retry succeeds. A first network rejected by WiFiMulti remains usable directly. If another network is later accepted, try WiFiMulti first and fall back to the direct network when the scan fails.
- Additional networks retain WiFiMulti selection behavior and do not replace the
  first direct-network snapshot.
- The direct Enterprise path owns SSID, username, password, identity and CA text;
  null/empty identity defaults to a copied username. Allocation failure preserves
  the existing personal-network fallback.
- Tracked credential allocations are released on replacement and destruction.

The WiFiMulti mock models its owning network list and controllable rejection;
it does not exercise a real WiFi driver. Enterprise calls are tested with mocks,
not authenticated against an actual EAP server. The allocation override targets
the credential storage allocator; it is not a general memory sanitizer.

For a negative control, compile the same test with the unmodified 2.49.0 `src`
directory on the include path instead of `-Isrc`. The generic and ESP32 tests
fail after the original SSID array is modified. Running the enterprise binary
with the `enterprise` argument isolates the original Enterprise lifetime bug.
