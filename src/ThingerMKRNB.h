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

#define NB_CONNECTION_TIMEOUT   120000
#define GPRS_CONNECTION_TIMEOUT 60000

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


    void connect_socket_success() override{
        connection_errors_ = 0;
    }

    void connect_socket_error() override{
        connection_errors_++;
        if(connection_errors_==6){
            // soft reset the modem if socket cannot be connected in a while
            THINGER_DEBUG("NETWORK", "Soft-Restarting modem...");
            MODEM.reset();
        }else if(connection_errors_==12){
            THINGER_DEBUG("NETWORK", "Hard-Restarting modem...");
            // cannot connect after several attempts? try to restart the modem
            MODEM.hardReset();
        }
    }

    bool network_connected() override{
        THINGER_DEBUG("__NBIOT", "checking...")
        // check CEREG
        return nbConnected_ ? nbAccess_.isAccessAlive() : false;
    }

    bool connect_network() override{
        THINGER_DEBUG("__NBIOT", "Initializing NB Network...")
        THINGER_DEBUG_VALUE("__NBIOT", "Connecting to APN: ", apn_)
        THINGER_DEBUG_VALUE("__NBIOT", "APN username: ", username_)
        THINGER_DEBUG_VALUE("__NBIOT", "APN password: ", password_)
        // set NB connection timeout
        nbAccess_.setTimeout(NB_CONNECTION_TIMEOUT);

        if(modemRestart_){
            THINGER_DEBUG("__NBIOT", "Modem will be restarted...");
        }

        // initialize modem, synchronously
        nbConnected_ = nbAccess_.begin(pin_, apn_, username_, password_, modemRestart_, true) == NB_READY;

        THINGER_DEBUG_VALUE("__NBIOT", "NB Network: ", nbConnected_)

        // will try to connect next time by restarting the modem
        if(!nbConnected_){
            THINGER_DEBUG("__NBIOT", "Cannot connect, will try to restart modem next time...")
            modemRestart_ = true;
            return false;
        }

        // connection is ok reset restart flag for next time
        modemRestart_ = false;

        // try to attach GPRS
        THINGER_DEBUG("___GPRS", "Initializing GPRS Connection...")
        // set GPRS connection timeout
        gprs_.setTimeout(GPRS_CONNECTION_TIMEOUT);
        // try to attach GPRS, synchronously
        bool gprsConnected = gprs_.attachGPRS(true) == GPRS_READY;
        THINGER_DEBUG_VALUE("___GPRS", "GPRS Connection: ", gprsConnected);
        if(!gprsConnected) return false;
        THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", gprs_.getIPAddress())

        return true;
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
    const char * pin_           = nullptr;
    const char * apn_           = nullptr;
    const char * username_      = nullptr;
    const char * password_      = nullptr;
    bool nbConnected_           = false;
    bool modemRestart_          = false;
    uint8_t connection_errors_  = 0;


#ifndef _DISABLE_TLS_
    NBSSLClient client_;
#else
    NBClient client_;
#endif

    GPRS gprs_;
    NB nbAccess_;

};

#endif