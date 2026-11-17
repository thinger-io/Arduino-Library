# Regression test for report item 7

This host test includes the production headers with small Arduino/transport mocks.
The FreeRTOS mock checks ownership and uses real host threads; it is not an ESP32 hardware test.
From the repository root, run:

```sh
c++ -std=c++11 -pthread -D__arm__ -Itests/regression_7/support -Isrc tests/regression_7/main.cpp -o regression_7
./regression_7
```

Do not use `-DNDEBUG`: assertions are the test oracle.
