# Allow slow cellular registration across reconnect attempts

The simulated modem regression passed for slow registration and for resetting after the configured number of unsuccessful windows.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/modem-registration/support -Isrc tests/modem-registration/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
