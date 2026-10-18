// The MIT License (MIT)
//
// Copyright (c) 2026 THINK BIG LABS SL
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

#ifndef THINGER_UNOQ_H
#define THINGER_UNOQ_H

// Arduino UNO Q: the MCU has no network, so the connection goes through the Linux side
// (arduino-router) with the TCP client of Arduino_RouterBridge (Library Manager). TLS is done
// on Linux, so its network (i.e. nmcli) and clock (NTP) must be ready to connect.
#include <Arduino_RouterBridge.h>
#include <ThingerClient.h>

class ThingerUnoQ : public ThingerClient {

public:
    ThingerUnoQ(const char* user, const char* device, const char* device_credential) :
        ThingerClient(client_, user, device, device_credential),
        client_(Bridge)
    {}

protected:

    // the network is managed by the Linux side
    bool network_connected() override{
        return bridge_ready_;
    }

    bool connect_network() override{
        if(!bridge_ready_) bridge_ready_ = client_.begin();
        return bridge_ready_;
    }

    // BridgeTCPClient returns 0 on success (instead of 1 as the Arduino Client)
    bool connect_socket() override{
#ifdef _DISABLE_TLS_
        return client_.connect(get_host(), THINGER_PORT) == 0;
#else
        return client_.connectSSL(get_host(), THINGER_SSL_PORT, get_root_ca()) == 0;
#endif
    }

    // BridgeTCPClient only fetches data from Linux on available(), so read() must call it
    bool read(char* buffer, size_t size) override{
        unsigned long start = millis();
        size_t total_read = 0;
        while(total_read<size){
            if(client_.available()){
                total_read += client_.read((uint8_t*)buffer+total_read, size-total_read);
            }else if(!client_.connected() || millis()-start>=DEFAULT_READ_TIMEOUT){
                THINGER_DEBUG("_SOCKET", "Cannot read from socket!");
                return false;
            }
        }
        return true;
    }

private:
    BridgeTCPClient<> client_;
    bool bridge_ready_ = false;
};

#endif
