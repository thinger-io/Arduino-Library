# Clear event subscriptions on disconnect

The host protocol regression passed after subscribing an event-only resource, clearing streams and checking the resource state and sampled counter.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/event-streams/support -Isrc tests/event-streams/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
