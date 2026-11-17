# Fix cooperative shutdown of the ESP32 FreeRTOS worker

## Regression scenario

In 2.49.0, stop() clears running_, yields, and externally deletes the worker. A legal schedule allows the worker to observe the cleared flag and return from its entry function before the deletion. ESP-IDF aborts when a task entry function returns. External deletion can also interrupt handle() while it owns synchronization or transport resources.

For example, call start() followed immediately by ThingerESP32FreeRTOS::stop() before the worker first runs: the yield can schedule a task whose loop condition is already false.

## Run

From the repository root, using a C++11 compiler:

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/freertos_lifecycle/support -Isrc tests/freertos_lifecycle/main.cpp -o regression
./regression
~~~

On Windows use regression.exe. zig c++ (tested with Zig 0.13.0) may replace c++.
Do not define NDEBUG: assertions are the oracle. __arm__ selects a mocked reset
implementation; this is a host build, not a cross-compilation.

For a negative control, compile this same test against the unmodified 2.49.0
src directory (commit c6f62e01e80b5e7379fd7b41322586f295773a36).
The original version compiles but fails the regression assertions.

## Coverage

Stop before first scheduling; stop while a mocked client operation is blocked; callback-requested stop; rejection of restart from the worker; restart after completion; repeated stop; task creation failure; semaphore allocation failure. A threaded FreeRTOS double rejects external task deletion and records unexpected task-function returns.

## Scope and limitations

External stop() now waits for handle() to return; it is not a forced cancellation API and its latency depends on network operations and user callbacks. Calling stop() from a worker callback requests shutdown asynchronously. Stop explicitly from another task before destroying the owning network adapter/client; destruction from the worker callback is unsupported. This does not address the separate recursive protocol-mutex issue. Two FreeRTOS synchronization allocations are added per wrapper.

The tests include the production headers and small platform/transport doubles.
No physical board or real service account is required.
