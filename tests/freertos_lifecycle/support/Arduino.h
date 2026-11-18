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
extern void test_yield(); inline void yield(){test_yield();}
inline unsigned long micros(){return millis()*1000;}
#include <condition_variable>
#include <vector>
#include <memory>
#include <future>
using BaseType_t=int;
struct TaskRecord {std::thread thread;};
using TaskHandle_t=TaskRecord*;
static thread_local TaskHandle_t current_task=reinterpret_cast<TaskHandle_t>(1);
static std::vector<std::unique_ptr<TaskRecord>> tasks;
static std::atomic<int> returned{0},self_deleted{0};
static std::atomic<bool> hold_entry{false},fail_create{false},fail_semaphore{false};
struct SelfDeleted {};
struct TestSemaphore {
    std::mutex mutex;std::condition_variable changed;int count;
    std::recursive_mutex recursive; // used by xSemaphore*Recursive (thinger.h, PR #64)
    explicit TestSemaphore(int initial):count(initial){}
};
using SemaphoreHandle_t=TestSemaphore*;
#define portMAX_DELAY 0xffffffffU
#define pdPASS 1
#define pdTRUE 1
using TickType_t=unsigned;
inline TickType_t xTaskGetTickCount(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
#define configASSERT(x) assert(x)
inline SemaphoreHandle_t xSemaphoreCreateMutex(){
    if(fail_semaphore.exchange(false))return nullptr;
    return new TestSemaphore(1);
}
inline SemaphoreHandle_t xSemaphoreCreateBinary(){return new TestSemaphore(0);}
static std::atomic<int> indefinite_waiters{0};
inline int xSemaphoreTake(SemaphoreHandle_t s,unsigned timeout){
    std::unique_lock<std::mutex> guard(s->mutex);
    if(s->count==0){
        if(timeout==portMAX_DELAY){
            ++indefinite_waiters;
            s->changed.wait(guard,[&]{return s->count>0;});
            --indefinite_waiters;
        }else if(!s->changed.wait_for(guard,std::chrono::milliseconds(timeout),[&]{return s->count>0;}))return 0;
    }
    --s->count;return 1;
}
inline int xSemaphoreGive(SemaphoreHandle_t s){
    std::lock_guard<std::mutex> lock(s->mutex);
    ++s->count;s->changed.notify_one();return 1;
}
inline void vSemaphoreDelete(SemaphoreHandle_t s){delete s;}
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(){return new TestSemaphore(1);}
inline int xSemaphoreTakeRecursive(SemaphoreHandle_t s,unsigned){s->recursive.lock();return 1;}
inline int xSemaphoreGiveRecursive(SemaphoreHandle_t s){s->recursive.unlock();return 1;}
inline TaskHandle_t xTaskGetCurrentTaskHandle(){return current_task;}
inline void vTaskDelay(unsigned){std::this_thread::sleep_for(std::chrono::milliseconds(1));}
inline void vTaskDelete(TaskHandle_t handle){
    if(handle!=nullptr)throw std::runtime_error("unexpected external task deletion");
    ++self_deleted;throw SelfDeleted{};
}
inline BaseType_t xTaskCreateUniversal(void(*fn)(void*),const char*,size_t,void* arg,int,TaskHandle_t* handle,unsigned){
    if(fail_create.exchange(false))return 0;
    std::unique_ptr<TaskRecord> record(new TaskRecord);
    TaskHandle_t ptr=record.get();if(handle)*handle=ptr;
    record->thread=std::thread([=]{
        current_task=ptr;
        while(hold_entry.load())vTaskDelay(1);
        try{fn(arg);++returned;}catch(const SelfDeleted&){}
    });
    tasks.push_back(std::move(record));return pdPASS;
}
inline void join_tasks(){for(auto& t:tasks)if(t->thread.joinable())t->thread.join();tasks.clear();}
template<class Predicate> void wait_until(Predicate check){
    for(int i=0;i<5000;++i){if(check())return;vTaskDelay(1);}
    assert(!"timed out");
}
