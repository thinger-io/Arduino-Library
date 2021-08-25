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

#ifndef THINGER_MKRNB_H
#define THINGER_MKRNB_H


#define GPRS_CONNECTION_TIMEOUT 30000

#include <MKRNB.h>
#include "ThingerClient.h"

class ThingerMKRNB : public ThingerClient {

public:
    ThingerMKRNB(const char* user, const char* device, const char* device_credential) :
        ThingerClient(client_, user, device, device_credential)
    {
        //MODEM.debug();
    }

    ~ThingerMKRNB(){

    }

protected:

    virtual bool network_connected(){
        return nbConnected_ && gprsConnected_;
    }

    virtual bool connect_network(){
        if(!nbConnected_){
            THINGER_DEBUG("__NBIOT", "Initializing NB Network...")
            THINGER_DEBUG_VALUE("__NBIOT", "Connecting to APN: ", apn_)
            THINGER_DEBUG_VALUE("__NBIOT", "APN username: ", username_)
            THINGER_DEBUG_VALUE("__NBIOT", "APN password: ", password_)
            nbConnected_ = nbAccess_.begin(pin_, apn_, username_, password_, false, true) == NB_READY;
            THINGER_DEBUG_VALUE("__NBIOT", "NB Network: ", nbConnected_)
        }

        if(nbConnected_){
            // try to attach GPRS
            THINGER_DEBUG("___GPRS", "Initializing GPRS Connection...")
            gprs_.setTimeout(GPRS_CONNECTION_TIMEOUT);
            gprsConnected_ = gprs_.attachGPRS(true) == GPRS_READY;
            THINGER_DEBUG_VALUE("___GPRS", "GPRS Connection: ", gprsConnected_);
            if(gprsConnected_){
                THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", gprs_.getIPAddress())
            }
            
        }

        return nbConnected_ && gprsConnected_;
    }

public:

    void set_apn(const char* apn, const char* apn_username="", const char* apn_password="")
    {
        apn_ = apn;
        username_ = apn_username;
        password_ = apn_password;
    }

    void set_pin(const char* pin){
        pin_ = pin;
    }

    GPRS& getGPRS(){
        return gprs_;
    }

    NB& getNB(){
        return nbAccess_;
    }

protected:
    const char * pin_       = nullptr;
    const char * apn_       = nullptr;
    const char * username_  = nullptr;
    const char * password_  = nullptr;
    bool nbConnected_       = false;
    bool gprsConnected_     = false;

#ifndef _DISABLE_TLS_
    NBSSLClient client_;
#else
    NBClient client_;
#endif

    GPRS gprs_;
    NB nbAccess_;

};

#endif