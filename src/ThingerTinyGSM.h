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


#ifndef THINGER_TINY_GSM_H
#define THINGER_TINY_GSM_H

#include "ThingerClient.h"

#define NETWORK_REGISTER_TIMEOUT 15000
#define NETWORK_GPRS_TIMEOUT 15000

class ThingerTinyGSM : public ThingerClient {

public:
    ThingerTinyGSM(const char* user, const char* device, const char* device_credential, Stream& serial) :
            apn_(NULL),
            user_(NULL),
            password_(NULL),
            pin_(NULL),
            modem_(serial),
            client_(modem_),
            ThingerClient(client_, user, device, device_credential)
    {
    }

    ~ThingerTinyGSM(){

    }

protected:

    virtual bool network_connected(){
        return modem_.isNetworkConnected() && modem_.isGprsConnected();
    }

    virtual bool connect_network(){
        if(apn_==NULL) {
            THINGER_DEBUG("NETWORK", "Cannot connect without setting the APN!")
            return false;
        }

        if(!modem_.isNetworkConnected()){
            THINGER_DEBUG("NETWORK", "Initializing Modem...")
            if(!modem_.restart()){
                THINGER_DEBUG("NETWORK", "Cannot init modem! Is it connected?")
                return false;
            }else{
                if(pin_!=NULL){
                    THINGER_DEBUG_VALUE("NETWORK", "Unlocking SIM... ", pin_)
                    if(!modem_.simUnlock(pin_)){
                        THINGER_DEBUG("NETWORK", "Cannot unlock SIM!")
                        return false;
                    }
                }

                THINGER_DEBUG("NETWORK", "Waiting for Network...")
                if(!modem_.waitForNetwork(NETWORK_REGISTER_TIMEOUT)) {
                    THINGER_DEBUG("NETWORK", "Cannot connect network!")
                    return false;
                }
                THINGER_DEBUG("NETWORK", "Network Connected!")
            }
        }

        if(!modem_.isGprsConnected()){
            THINGER_DEBUG("NETWORK", "Connecting to APN...")
            if (!modem_.gprsConnect(apn_, user_, password_)) {
                THINGER_DEBUG("NETWORK", "Cannot connect to APN!")
                return false;
            }
        }

        return true;
    }

    virtual bool secure_connection(){
        return false;
    }

public:
    TinyGsm& getTinyGsm(){
        return modem_;
    }

    TinyGsmClient& getTinyGsmClient(){
        return client_;
    }

    void setAPN(const char* APN, const char* user=NULL, const char* password=NULL){
        apn_ = APN;
        user_ = user;
        password_ = password;
    }

    void setPIN(const char* pin){
        pin_ = pin;
    }

private:
    const char* apn_;
    const char* user_;
    const char* password_;
    const char* pin_;
    TinyGsm modem_;
    TinyGsmClient client_;
};

#endif