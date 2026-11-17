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
        if(running_.load()){
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

    bool stop(){
        if(worker_.load()==xTaskGetCurrentTaskHandle()){
            running_.store(false);
            return true; // Finish the callback and leave handle() before deleting this task.
        }
        if(!lifecycle_mutex_ || !stopped_) return true;
        xSemaphoreTake(lifecycle_mutex_, portMAX_DELAY);
        running_.store(false);
        if(task_started_){
            xSemaphoreTake(stopped_, portMAX_DELAY);
            task_started_ = false;
        }
        xSemaphoreGive(lifecycle_mutex_);
        return true;
    }

    bool is_running(){
        return running_.load();
    }

private:
    ThingerClient& task_client_;
    SemaphoreHandle_t lifecycle_mutex_;
    SemaphoreHandle_t stopped_;
    std::atomic<TaskHandle_t> worker_{nullptr};
    std::atomic<bool> running_{false};
    bool task_started_ = false;

};

#endif