# Reject OTA begin requests when updates are disabled

## Regression scenario

set_enabled(false) makes the OTA options resource advertise disabled, but the $ota/begin handler does not check that flag. An authenticated caller sending the command directly can still invoke begin_ota(), obtain success, and start an update. The advertised capability is not enforced at the command boundary.

## Run

From the repository root, using a C++11 compiler:

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/ota_enabled/support -Isrc tests/ota_enabled/main.cpp -o regression
./regression
~~~

On Windows use regression.exe. zig c++ (tested with Zig 0.13.0) may replace c++.
Do not define NDEBUG: assertions are the oracle. __arm__ selects a mocked reset
implementation; this is a host build, not a cross-compilation.

For a negative control, compile this same test against the unmodified 2.49.0
src directory (commit c6f62e01e80b5e7379fd7b41322586f295773a36).
The original version compiles but fails the regression assertions.

## Coverage

Disabled options response; rejection without invoking begin_ota() or changing running state; re-enabling allows a normal begin; duplicate begin remains rejected.

## Scope and limitations

This controls new updates, not cancellation of an update already in progress. The test executes production resource handlers with a mock updater and checksums/compression disabled; it does not simulate transport authentication, flash writes, or the blocking post-response loop. This is not a claim of unauthenticated access.

The tests include the production headers and small platform/transport doubles.
No physical board or real service account is required.
