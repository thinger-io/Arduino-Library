# Clear active subscriptions on nested resources

The host regression passed for subscriptions on child resources and cleanup of the sampled counter.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/nested-streams/support -Isrc tests/nested-streams/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
