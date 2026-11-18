// The MIT License (MIT)
//
// Copyright (c) 2017 THINK BIG LABS SL
// Author: alvarolb@gmail.com (Alvaro Luis Bustamante)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef THINGER_ESP32_FREERTOS_H
#define THINGER_ESP32_FREERTOS_H

// ThingerClient.h must be compiled with multitask support: it is not if another header of the library (i.e.,
// ThingerESP32OTA.h or ThingerConsole.h) was included before ThingerESP32.h or ThingerTinyGSM.h
#if defined(THINGER_CLIENT_H) && !defined(THINGER_MULTITASK)
#error "With THINGER_FREE_RTOS, include ThingerESP32.h (or ThingerTinyGSM.h, ThingerESP32Eth.h) before other thinger.io headers"
#endif

// enable thinger library with multitask support
#define THINGER_MULTITASK
#define THINGER_FREE_RTOS_MULTITASK

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <atomic>
#include "ThingerClient.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "esp_task_wdt.h"

class ThingerESP32FreeRTOS {

public:
    ThingerESP32FreeRTOS(ThingerClient& client)
        : task_client_(client),
          lifecycle_mutex_(xSemaphoreCreateMutex()),
          stopped_(xSemaphoreCreateBinary())
    {
    }

    // Stop the worker before destroying the owning network adapter or its client.
    virtual ~ThingerESP32FreeRTOS(){
        stop();
        if(stopped_) vSemaphoreDelete(stopped_);
        if(lifecycle_mutex_) vSemaphoreDelete(lifecycle_mutex_);
    }

    bool start(unsigned core=ARDUINO_RUNNING_CORE, size_t stack_size=8192){
        // A worker cannot join its own previous run.
        if(worker_.load()==xTaskGetCurrentTaskHandle()) return false;
        if(!lifecycle_mutex_ || !stopped_) return false;
        xSemaphoreTake(lifecycle_mutex_, portMAX_DELAY);
        if(running_.load() || closing_.load()){
            xSemaphoreGive(lifecycle_mutex_);
            return false;
        }
        // A stop requested by a worker callback completes asynchronously.
        if(task_started_){
            xSemaphoreTake(stopped_, portMAX_DELAY);
            task_started_ = false;
        }
        running_.store(true);
        BaseType_t result = xTaskCreateUniversal(
            [](void* param){
                auto* instance = static_cast<ThingerESP32FreeRTOS*>(param);
                instance->worker_.store(xTaskGetCurrentTaskHandle());
                while(instance->running_.load()){
                    vTaskDelay(5);
                    if(instance->running_.load()) instance->task_client_.handle();
                }
                if(instance->close_on_exit_){
                    instance->close_on_exit_ = false;
                    instance->task_client_.stop();
                }
                // Do not access the instance after publishing completion.
                SemaphoreHandle_t stopped = instance->stopped_;
                instance->worker_.store(nullptr);
                xSemaphoreGive(stopped);
                vTaskDelete(nullptr);
            },
            "thinger.io", stack_size, this, 1, nullptr, core
        );
        task_started_ = result == pdPASS;
        if(!task_started_) running_.store(false);
        xSemaphoreGive(lifecycle_mutex_);
        return result == pdPASS;
    }

    // Stops the task, waiting up to timeout for the current handle() to finish (it may take long while
    // connecting, or while the WebConfig portal is open). Returns false on timeout: the task stops later.
    // Do not call it while holding thing.lock(), as the task may be waiting for that lock.
    bool stop(TickType_t timeout=portMAX_DELAY){
        return stop_worker(timeout, false);
    }

protected:
    bool stop_worker(TickType_t timeout, bool close_connection){
        if(worker_.load()==xTaskGetCurrentTaskHandle()){
            close_on_exit_ |= close_connection;
            running_.store(false);
            return true; // Finish the callback and leave handle() before deleting this task.
        }
        if(!lifecycle_mutex_ || !stopped_){
            if(close_connection) task_client_.stop();
            return true;
        }
        TickType_t started = xTaskGetTickCount();
        if(xSemaphoreTake(lifecycle_mutex_, timeout)!=pdTRUE) return false;
        running_.store(false);
        bool stopped = true;
        if(task_started_){
            if(timeout!=portMAX_DELAY){
                TickType_t elapsed = xTaskGetTickCount() - started;
                timeout = elapsed < timeout ? timeout-elapsed : 0;
            }
            stopped = xSemaphoreTake(stopped_, timeout)==pdTRUE;
            // on timeout, the next start() or stop() waits for the task to finish
            if(stopped) task_started_ = false;
        }
        bool close_now = stopped && close_connection && !closing_.exchange(true);
        xSemaphoreGive(lifecycle_mutex_);
        if(close_now){
            // State listeners may call back into lifecycle APIs.
            task_client_.stop();
            closing_.store(false);
        }
        return stopped;
    }

public:
    bool is_running(){
        return running_.load();
    }

private:
    ThingerClient& task_client_;
    SemaphoreHandle_t lifecycle_mutex_;
    SemaphoreHandle_t stopped_;
    std::atomic<TaskHandle_t> worker_{nullptr};
    std::atomic<bool> running_{false};
    std::atomic<bool> closing_{false};
    bool close_on_exit_ = false; // Only used by the worker.
    bool task_started_ = false;

};

#endif