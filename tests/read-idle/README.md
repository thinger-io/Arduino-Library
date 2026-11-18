# Measure socket read timeout from the last received byte

The host regression passed with a scripted slow transport and a simulated clock.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/read-idle/support -Isrc tests/read-idle/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
