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

#ifndef THINGER_MBED_ETH_H
#define THINGER_MBED_ETH_H

#include <mbed.h>

// enable thinger library with multitask support
#define THINGER_MULTITASK
#define THINGER_MBED_MULTITASK

#include <Ethernet.h>
#include <PortentaEthernet.h>

// define the client type based on the TLS support
#ifdef _DISABLE_TLS_
typedef EthernetClient MBEDClient;
#else
#include <EthernetSSLClient.h>
typedef EthernetSSLClient MBEDClient;
#endif

#include "ThingerClient.h"


class ThingerMbedEth : public ThingerClient{

public:
    ThingerMbedEth(const char* user, const char* device, const char* device_credential) :
        ThingerClient(client_, user, device, device_credential)
    {
        
    }

    ~ThingerMbedEth(){

    }

    bool start(){
        if(running_) return false;
        running_ = true;
        get_thread().start([this](){
            THINGER_DEBUG("MBED_OS", "Starting Thinger.io task...")
            while(running_){
                rtos::ThisThread::yield();
                this->handle();
            }
        });
        return true;
    }

    bool stop(){
        if(running_){
            THINGER_DEBUG("MBED_OS", "Stopping Thinger.io task...")
            running_ = false;
            get_thread().join();
        }
        return true;
    }

    bool is_running(){
        return running_;
    }

protected:

    rtos::Thread& get_thread(){
        static rtos::Thread thread;
        return thread;
    }

    bool network_connected() override{
        return initialized_ ? Ethernet.linkStatus() == LinkON : false;
    }

    bool connect_network() override{
        if(!initialized_){

            // Check for Ethernet hardware present
            if (Ethernet.hardwareStatus() == EthernetNoHardware) {
                THINGER_DEBUG("NETWORK", "Ethernet shield was not found.");
                return false;
            }

            if (Ethernet.begin() == 0) {
                THINGER_DEBUG("NETWORK", "Ethernet begin failed.");
                return false;
            }

            if (Ethernet.linkStatus() == LinkOFF) {
                THINGER_DEBUG("NETWORK", "Ethernet cable is not connected.");
                return false;;
            }

            THINGER_DEBUG("NETWORK", "Initializing Ethernet...");

            if (Ethernet.begin() == 0) {
                THINGER_DEBUG("NETWORK", "Failed to configure Ethernet using DHCP");
                return false;
            }

            THINGER_DEBUG_VALUE("NETWORK", "ETH MAC: ",         Ethernet.macAddress());
            THINGER_DEBUG_VALUE("NETWORK", "ETH IP: ",          Ethernet.localIP());

            // set as initialized
            initialized_ = true;

            // give the Ethernet shield a second to initialize:
            delay(1000);

            return initialized_;
        }

        return initialized_;
    }

private:
    MBEDClient client_;
    bool running_ = false;
    bool initialized_ = false;

};

#endif