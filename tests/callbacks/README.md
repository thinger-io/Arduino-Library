# Serialize resource callbacks across dispatch and sampling

The host regression checked lock ownership inside callbacks invoked by an incoming request, periodic sampling and bucket publication, and confirmed that the lock was released afterward.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/callbacks/support -Isrc tests/callbacks/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.

Synchronous request, sampling and publication callbacks may re-enter the protocol on the same task. A stop from another task must not wait for the worker while holding the protocol mutex. An incoming request handled during a synchronous ACK wait can inherit the outer lock, including in post-response hooks. A lengthy callback or OTA operation on this path can delay other tasks.
