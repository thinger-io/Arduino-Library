# Allow locally authorized replacement of rejected WebConfig credentials

The host suite checked local authorization, transport failure, blank secret display, replacement and fixed credentials. The ESP8266 WebConfig header passed a core 3.1.2 compiler check with WiFiManager.

From the repository root, build with a C++11 compiler and assertions enabled.

~~~sh
c++ -std=c++11 -pthread -D__arm__ -Itests/auth-recovery/support -Isrc tests/auth-recovery/main.cpp -o regression
./regression
~~~

The fixture uses production headers with host-side transport and platform doubles. The __arm__ define selects the stubbed reset path.
