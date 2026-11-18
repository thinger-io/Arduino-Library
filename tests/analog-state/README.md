# Keep analog output state per resource binding

The host regression passed with two independently written and queried analog resources.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/analog-state/support -Isrc tests/analog-state/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
