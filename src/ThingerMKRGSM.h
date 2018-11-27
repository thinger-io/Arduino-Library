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

#ifndef THINGER_MKR_GSM_H
#define THINGER_MKR_GSM_H

#include <MKRGSM.h>

#define GPRS_CONNECTION_TIMEOUT 30000
#define MODEM_RESPONSE_TIMEOUT 30000

#include "ThingerClient.h"

class ThingerMKRGSM : public ThingerClient {

public:
    ThingerMKRGSM(const char* user, const char* device, const char* device_credential) :
        ThingerClient(client_, user, device, device_credential),
        pin_(NULL),
        apn_(NULL),
        username_(NULL),
        password_(NULL),
        gsmConnected_(false),
        gprsConnected_(false)
    {
        //MODEM.debug();
    }

    ~ThingerMKRGSM(){

    }

protected:

    virtual bool network_connected(){
        return gsmConnected_ && gprsConnected_;
    }

    virtual bool connect_network(){
        if(!gsmConnected_){
            THINGER_DEBUG("SIMCARD", "Initializing GSM Network!")
            gsmConnected_ = gsmAccess_.begin(pin_, true, true) == GSM_READY;
            THINGER_DEBUG_VALUE("SIMCARD", "GSM Network: ", gsmConnected_)
        }

        if(gsmConnected_){
            if(apn_==NULL){
                THINGER_DEBUG("___GPRS", "APN was not set!")
                return false;
            }
            THINGER_DEBUG_VALUE("___GPRS", "Connecting to APN: ", apn_)
            THINGER_DEBUG_VALUE("___GPRS", "APN username: ", username_)
            THINGER_DEBUG_VALUE("___GPRS", "APN password: ", password_)
            unsigned long timeout = millis();
            // try to attach GPRS
            gprs_.attachGPRS(apn_, username_, password_, false);
            while(gprs_.ready() == 0) {
                if(millis() - timeout > GPRS_CONNECTION_TIMEOUT){
                    THINGER_DEBUG("___GPRS", "Cannot establish connection with APN!")
                    return false;
                }
                delay(100);
            }
            gprsConnected_ = true;
            THINGER_DEBUG("___GPRS", "APN Connection suceed!")
            THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", gprs_.getIPAddress());
        }

        return gsmConnected_ && gprsConnected_;
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

    GSM& getGSM(){
        return gsmAccess_;
    }

protected:
    bool gsmConnected_;
    bool gprsConnected_;
    const char * pin_;
    const char * apn_;
    const char * username_;
    const char * password_;

#ifndef _DISABLE_TLS_
    GSMSSLClient client_;
#else
    GSMClient client_;
#endif

    GPRS gprs_;
    GSM gsmAccess_;
};

#endif