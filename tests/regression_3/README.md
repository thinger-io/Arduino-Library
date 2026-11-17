# Regression test for report item 3

This host test includes the production headers with small Arduino/transport mocks.
The FreeRTOS mock checks ownership and uses real host threads; it is not an ESP32 hardware test.
From the repository root, run:

```sh
c++ -std=c++11 -pthread -D__arm__ -Itests/regression_3/support -Isrc tests/regression_3/main.cpp -o regression_3
./regression_3
```

Do not use `-DNDEBUG`: assertions are the test oracle.
