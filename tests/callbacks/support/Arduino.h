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
// Model task ownership, recursion and mutual exclusion. Old self-deadlocks
// throw instead of hanging the regression process forever.
struct TestSemaphore {
    std::recursive_mutex native;
    unsigned depth=0;
    bool recursive;
    explicit TestSemaphore(bool r):recursive(r){}
};
using SemaphoreHandle_t=TestSemaphore*;
#define portMAX_DELAY 0xffffffffU
#define configASSERT(x) assert(x)
inline SemaphoreHandle_t xSemaphoreCreateMutex(){return new TestSemaphore(false);}
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(){return new TestSemaphore(true);}
inline int take(SemaphoreHandle_t m,bool recursive){
    m->native.lock();
    if(m->depth && !recursive){m->native.unlock(); throw std::runtime_error("non-recursive self-deadlock");}
    ++m->depth; return 1;
}
inline int give(SemaphoreHandle_t m){assert(m->depth); --m->depth;m->native.unlock();return 1;}
inline int xSemaphoreTake(SemaphoreHandle_t m,unsigned){assert(!m->recursive);return take(m,false);}
inline int xSemaphoreGive(SemaphoreHandle_t m){assert(!m->recursive);return give(m);}
static thread_local unsigned callback_depth=0;
inline int xSemaphoreTakeRecursive(SemaphoreHandle_t m,unsigned){assert(m->recursive);++callback_depth;return take(m,true);}
inline int xSemaphoreGiveRecursive(SemaphoreHandle_t m){assert(m->recursive);--callback_depth;return give(m);}
inline void vSemaphoreDelete(SemaphoreHandle_t m){delete m;}
inline unsigned long micros(){return millis()*1000;}
