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

#ifdef THINGER_FREE_RTOS
#include "ThingerESP32FreeRTOS.h"
#endif

#include "ThingerClient.h"

#define NETWORK_REGISTER_TIMEOUT 15000
#define NETWORK_GPRS_TIMEOUT 15000

class ThingerTinyGSM : public ThingerClient

#ifdef THINGER_FREE_RTOS
,public ThingerESP32FreeRTOS
#endif

{

public:
    ThingerTinyGSM(const char* user, const char* device, const char* device_credential, Stream& serial) :
        ThingerClient(client_, user, device, device_credential),
        #ifdef THINGER_FREE_RTOS
        ThingerESP32FreeRTOS(static_cast<ThingerClient&>(*this)),
        #endif
        modem_(serial),
        client_(modem_)
    {
    }

    ~ThingerTinyGSM(){

    }

protected:

    virtual bool network_connected(){
        return modem_.isNetworkConnected() && modem_.isGprsConnected();
    }

    virtual bool connect_network(){
        if(apn_==nullptr) {
            THINGER_DEBUG("NETWORK", "Cannot connect without setting the APN!")
            set_error(NETWORK_NOT_CONFIGURED);
            return false;
        }

        if(module_reset_) module_reset_();

        if(!modem_.isNetworkConnected()){
            THINGER_DEBUG("NETWORK", "Initializing Modem...")
            if(!modem_.restart()){
                THINGER_DEBUG("NETWORK", "Cannot init modem! Is it connected?")
                set_error(MODEM_NOT_RESPONDING);
                return false;
            }else{
                THINGER_DEBUG_VALUE("NETWORK", "Modem Info: ", modem_.getModemInfo());

                // Unlock your SIM card with a PIN if needed
                if(pin_!=nullptr && modem_.getSimStatus() != 3){
                    THINGER_DEBUG_VALUE("NETWORK", "Unlocking SIM... ", pin_)
                    if(!modem_.simUnlock(pin_)){
                        THINGER_DEBUG("NETWORK", "Cannot unlock SIM!")
                        set_error(MODEM_SIM_ERROR);
                        return false;
                    }
                }

                THINGER_DEBUG("NETWORK", "Waiting for Network...")
                if(!modem_.waitForNetwork(NETWORK_REGISTER_TIMEOUT)) {
                    THINGER_DEBUG("NETWORK", "Cannot connect network!")
                    set_error(NETWORK_NOT_REGISTERED);
                    return false;
                }
                THINGER_DEBUG("NETWORK", "Network Connected!")
            }
        }

        if(!modem_.isGprsConnected()){
            THINGER_DEBUG("NETWORK", "Connecting to APN...")
            if (!modem_.gprsConnect(apn_, user_, password_)) {
                THINGER_DEBUG("NETWORK", "Cannot connect to APN!")
                set_error(NETWORK_APN_ERROR);
                return false;
            }
        }

        return true;
    }

#ifdef _DISABLE_TLS_
    virtual bool secure_connection(){
        return false;
    }
#endif

public:
    TinyGsm& getTinyGsm(){
        return modem_;
    }

    TinyGsmClient& getTinyGsmClient(){
        return client_;
    }

    void setAPN(const char* APN, const char* user=nullptr, const char* password=nullptr){
        apn_ = APN;
        user_ = user;
        password_ = password;
    }

    void setPIN(const char* pin){
        pin_ = pin;
    }

#ifdef THINGER_USE_FUNCTIONAL
    void setModuleReset(std::function<void()> module_reset){
#else
    void setModuleReset(void (*module_reset)()){
#endif
        module_reset_ = module_reset;
    }

private:
    const char* apn_        = nullptr;
    const char* user_       = nullptr;
    const char* password_   = nullptr;
    const char* pin_        = nullptr;
    TinyGsm modem_;
#ifdef _DISABLE_TLS_
    TinyGsmClient client_;
#else
    TinyGsmClientSecure client_;
#endif
#ifdef THINGER_USE_FUNCTIONAL
    std::function<void()> module_reset_;
#else
    void (*module_reset_)() = nullptr;
#endif

};

#endif