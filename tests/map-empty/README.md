# Report a one-entry resource map as nonempty

The host C++11 regression passed for empty and populated resource maps.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/map-empty/support -Isrc tests/map-empty/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
