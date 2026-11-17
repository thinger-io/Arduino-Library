# Unlock TinyGSM SIM cards only when the PIN is required

## Regression scenario

ThingerTinyGSM checks getSimStatus() != 3 before submitting a configured PIN. In TinyGSM 0.12.0, SIM_READY is 1 and 3 is SIM_ANTITHEFT_LOCKED. Consequently, an already-ready SIM receives an unnecessary AT+CPIN command. A modem that rejects that command causes connect_network() to report MODEM_SIM_ERROR even though the SIM is ready.

## Run

From the repository root, using a C++11 compiler:

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/tinygsm_pin/support -Isrc tests/tinygsm_pin/main.cpp -o regression
./regression
~~~

On Windows use regression.exe. zig c++ (tested with Zig 0.13.0) may replace c++.
Do not define NDEBUG: assertions are the oracle. __arm__ selects a mocked reset
implementation; this is a host build, not a cross-compilation.

For a negative control, compile this same test against the unmodified 2.49.0
src directory (commit c6f62e01e80b5e7379fd7b41322586f295773a36).
The original version compiles but fails the regression assertions.

## Coverage

Ready SIM with and without a configured PIN; PIN-locked SIM unlock; SIM_ERROR and SIM_ANTITHEFT_LOCKED do not trigger PIN submission. The modem double rejects PIN submission when it is not locked.

## Scope and limitations

The test uses the TinyGSM 0.12.0 enum contract and a modem double. It does not exercise real AT timing, cellular registration, SIM hardware, or modem-specific firmware responses.

The tests include the production headers and small platform/transport doubles.
No physical board or real service account is required.
