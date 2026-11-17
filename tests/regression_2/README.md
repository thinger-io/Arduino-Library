# Regression test for report item 2

This host test includes the production headers with small Arduino/transport mocks.
The FreeRTOS mock checks ownership and uses real host threads; it is not an ESP32 hardware test.
From the repository root, run:

```sh
c++ -std=c++11 -pthread -D__arm__ -Itests/regression_2/support -Isrc tests/regression_2/main.cpp -o regression_2
./regression_2
```

Do not use `-DNDEBUG`: assertions are the test oracle.
