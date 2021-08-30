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

#include "ThingerClient.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "esp_task_wdt.h"

class ThingerESP32FreeRTOS {

public:
    ThingerESP32FreeRTOS(ThingerClient& client)
        : task_client_(client)
    {

    }

    virtual ~ThingerESP32FreeRTOS(){

    }

    bool start(unsigned core=ARDUINO_RUNNING_CORE, size_t stack_size=8192){
        if(running_) return false;
        running_ = true;
        running_core_ = core;
        BaseType_t result = xTaskCreateUniversal(
            [](void* param){
                ThingerESP32FreeRTOS* instance = (ThingerESP32FreeRTOS*) param;
                while(instance->running_){
                    vTaskDelay(5);
                    instance->task_client_.handle();
                }
            },
            "thinger.io",
            stack_size,
            this,
            1,
            &task_handler_,
            core
        );
        running_ = result == pdPASS;
        return running_;
    }

    bool stop(){
        if(running_){
            running_ = false;
            yield();
            vTaskDelete(task_handler_);
        }
        return true;
    }

    bool is_running(){
        return running_;
    }

private:
    ThingerClient& task_client_;
    TaskHandle_t task_handler_ = nullptr;
    bool running_ = false;
    unsigned running_core_;
};

#endif