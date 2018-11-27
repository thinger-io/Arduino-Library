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

#ifndef THINGER_ESP8266_AT_H
#define THINGER_ESP8266_AT_H

// TinyGSM can be installed from Library Manager or https://github.com/vshymanskyy/TinyGSM
#define TINY_GSM_MODEM_ESP8266
#include <TinyGsmClient.h>
#include "ThingerClient.h"

class ThingerESP8266AT : public ThingerClient{

public:
    ThingerESP8266AT(const char* user, const char* device, const char* device_credential, Stream& serial) :
            serial_(serial),
            client_(serial_),
            wifi_ssid_(NULL),
            wifi_password_(NULL),
            ThingerClient(client_, user, device, device_credential)
    {}

    ~ThingerESP8266AT(){

    }

protected:

    virtual bool network_connected(){
        return serial_.isNetworkConnected();
    }

    virtual bool connect_network(){
        return serial_.networkConnect(wifi_ssid_, wifi_password_);
    }

    virtual bool secure_connection(){
        return false;
    }

public:

    void add_wifi(const char* ssid, const char* password=NULL)
    {
        wifi_ssid_ = ssid;
        wifi_password_ = password;
    }

protected:
    TinyGsm serial_;
    TinyGsm::GsmClient client_;
    const char* wifi_ssid_;
    const char* wifi_password_;

};

#endif