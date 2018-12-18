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

#ifndef THINGER_ETHERNET_H
#define THINGER_ETHERNET_H

#include <Ethernet.h>

#include "ThingerClient.h"

class ThingerEthernet : public ThingerClient {

public:
    ThingerEthernet(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential),
            connected_(false),
            connect_callback_(NULL)
    {}

    ~ThingerEthernet(){

    }

protected:

    virtual bool network_connected(){
        Ethernet.maintain();
        return connected_;
    }

    virtual bool connect_network(){
        if(connected_) return true;
        if(connect_callback_!=NULL){
            connected_ = connect_callback_();
        }else{
            byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
            unsigned long ethernet_timeout = millis();
            THINGER_DEBUG("NETWORK", "Initializing Ethernet...");
            while(Ethernet.begin(mac)==0){
                THINGER_DEBUG("NETWORK", "Getting IP Address...");
                if(millis() - ethernet_timeout > 30000) {
                    delay(1000);
                    return false;
                }
            }
            THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", Ethernet.localIP());
            delay(1000);
            connected_ = true;
        }
        return connected_;
    }

    virtual bool secure_connection(){
        return false;
    }

public:

    void set_network_setup(bool (*connect_callback)(void)){
        connect_callback_ = connect_callback;
    }

private:
    bool connected_;
    EthernetClient client_;
    bool (*connect_callback_)(void);

};

#endif