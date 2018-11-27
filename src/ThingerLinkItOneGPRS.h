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

#ifndef THINGER_LINKITONE_H
#define THINGER_LINKITONE_H

#include <LGPRS.h>
#include <LGPRSClient.h>

#include "ThingerClient.h"

class ThingerLinkItOneGPRS : public ThingerClient {

public:
    ThingerLinkItOneGPRS(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential), connected_(false),
            apn_(NULL), user_(NULL), password_(NULL), pin_(NULL)
    {}

    ~ThingerLinkItOneGPRS(){
    }

protected:

    virtual bool network_connected(){
        return connected_;
    }

    virtual bool connect_network(){
        unsigned long gprs_timeout = millis();
        THINGER_DEBUG("NETWORK", "Connecting to GPRS...");
        while(!attachGPRS())
        {
            if (millis() - gprs_timeout > 30000) return false;
            delay(500);
        }
        connected_ = true;
        THINGER_DEBUG("NETWORK", "Connected to GPRS!");
        return connected_;
    }

    virtual bool secure_connection(){
        return false;
    }

public:
    void set_apn(const char* apn, const char* user=NULL, const char* password=NULL){
        apn_ = apn;
        user_ = user;
        password_ = password;
    }

    void set_pin(const char* pin){
        pin_ = pin;
    }

private:
    bool connected_;
    LGPRSClient client_;
    const char* apn_;
    const char* user_;
    const char* password_;
    const char* pin_;

    bool attachGPRS(){
        if(apn_!=NULL){
            return LGPRS.attachGPRS(apn_, user_, password_) == 1;
        }
        else{
            return LGPRS.attachGPRS() == 1;
        }
    }
};

#endif