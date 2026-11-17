# Stop OTA processing immediately after rejecting a non-binary block

## Regression scenario

The $ota/write handler sets an error and clears running_ when the payload is not binary, but falls through into get_bytes() and write_ota(). An updater accepting a zero-length write can overwrite success=false with true and reactivate the session, confirming a block that was never valid.

## Run

From the repository root, using a C++11 compiler:

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/ota_payload/support -Isrc tests/ota_payload/main.cpp -o regression
./regression
~~~

On Windows use regression.exe. zig c++ (tested with Zig 0.13.0) may replace c++.
Do not define NDEBUG: assertions are the oracle. __arm__ selects a mocked reset
implementation; this is a host build, not a cross-compilation.

For a negative control, compile this same test against the unmodified 2.49.0
src directory (commit c6f62e01e80b5e7379fd7b41322586f295773a36).
The original version compiles but fails the regression assertions.

## Coverage

String, number, object, and empty payloads retain the invalid-type error, never invoke write_ota(), and leave the session stopped. Later writes are rejected. A valid binary block still reaches the updater and succeeds.

## Scope and limitations

Tests execute production resource handlers with a mock updater and checksums/compression disabled. They do not perform real flash writes or run the blocking post-response OTA loop. The change preserves the existing timeout/reboot handling after an aborted session.

The tests include the production headers and small platform/transport doubles.
No physical board or real service account is required.
