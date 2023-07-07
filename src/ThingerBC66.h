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

#ifndef THINGER_BC66_H
#define THINGER_BC66_H

#ifndef _DISABLE_TLS_
#include <LClientSecure.h>
#else
#include <LClient.h>
#endif

#include "ThingerClient.h"



class ThingerBC66 : public ThingerClient {

public:
    ThingerBC66(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential)
    {


    }

    ~ThingerBC66(){

    }

    static s32 callback(char* line, u32 len, void* userData){
        //THINGER_DEBUG("MODEM<<", line);
        return RIL_AT_SUCCESS;
    }

    static void send(const char* data, unsigned long timeout=1000){
        //THINGER_DEBUG("MODEM>>", data);
        Dev.send(data, timeout, callback);
        Ql_Sleep(100);
    }

protected:

    void connect_socket_success() override{
        connection_errors_ = 0;
    }

    void connect_socket_error() override{
        connection_errors_++;
        if(connection_errors_==6){
            // soft reset the modem if socket cannot be connected in a while
            THINGER_DEBUG("NETWORK", "Soft-Restarting modem...");
            send("AT+CFUN=0");
            send("AT+CFUN=1");
        }else if(connection_errors_==12){
            // still cannot connect after a soft-reset? -> hard reset
            THINGER_DEBUG("NETWORK", "Hard-Restarting modem...");
            Dev.reset();
        }
    }

    bool network_connected() override{
        auto cereg = Dev.cereg(false);
        THINGER_DEBUG_VALUE("NETWORK", "CEREG: ", cereg);
        return cereg == 1 || cereg == 5; // home & roaming
    }

    bool connect_network() override{
        THINGER_DEBUG("NETWORK", "Waiting for network...");
        unsigned long network_timeout = millis();
        while (1)
        {
            auto val = Dev.cereg(false);
            THINGER_DEBUG_VALUE("NETWORK", "CEREG: ", val);

            // home & roaming -> ok!
            if (1 == val || 5 == val){
                break;
            }

            // registration denied
            if(val==3){
                // wait some time to not flood operator if sim is legitimately disabled
                Ql_Sleep(30000);
                // reset device
                Dev.reset();
                return false;
            }

            // other ? i.e., 2, searching, or 0. reboot device if timed out
            if(millis() - network_timeout > 300000){
                Dev.reset();
                return false;
            }

            Ql_Sleep(500);
        }

        Ql_Sleep(100);
        THINGER_DEBUG("NETWORK", "Network registered...");
        return true;
    }

    virtual bool secure_connection(){
#ifndef _DISABLE_TLS_
        return true;
#else
        return false;
#endif
    }

protected:
    uint8_t connection_errors_ = 0;
#ifndef _DISABLE_TLS_
    LClientSecure client_;
#else
    LClient client_;
#endif
};

#endif