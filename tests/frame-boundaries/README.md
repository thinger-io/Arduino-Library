# Enforce declared protocol frame and container lengths during decoding

The host test decoded two valid consecutive frames containing nested data and simulated losing the connection partway through a third. The header also passed an ATmega328P compiler syntax check.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/frame-boundaries/support -Isrc tests/frame-boundaries/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
