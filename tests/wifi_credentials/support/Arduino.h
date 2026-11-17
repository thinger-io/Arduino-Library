#pragma once
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <chrono>
#include <atomic>
using String = std::string;
#define F(x) x
#define HIGH 1
#define LOW 0
inline int digitalRead(int){return 0;}
inline void digitalWrite(int,int){}
inline void analogWrite(int,int){}
inline void NVIC_SystemReset(){}
inline unsigned long millis(){static std::atomic<unsigned long> t{0}; return t.fetch_add(100);}
inline void delay(unsigned long){}
inline long random(long n){return n ? std::rand()%n : 0;}
inline void yield(){std::this_thread::yield();}
inline unsigned long micros(){return millis()*1000;}
