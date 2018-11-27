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

#ifndef THINGER_WIFI_H
#define THINGER_WIFI_H

#include "ThingerClient.h"

template <class Client>
class ThingerWifiClient : public ThingerClient {

public:
    ThingerWifiClient(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential),
            wifi_ssid_(NULL),
            wifi_password_(NULL)
    {}

    ~ThingerWifiClient(){

    }

protected:

    virtual bool network_connected(){
        return (WiFi.status() == WL_CONNECTED) && !(WiFi.localIP() == INADDR_NONE);
    }

    virtual bool connect_network(){
        if(wifi_ssid_==NULL){
            THINGER_DEBUG("NETWORK", "Cannot connect to WiFi. SSID not set!");
        }

        unsigned long wifi_timeout = millis();
        THINGER_DEBUG_VALUE("NETWORK", "Connecting to network ", wifi_ssid_);

        if(wifi_password_!=NULL){
            WiFi.begin((char*)wifi_ssid_, (char*) wifi_password_);
        }else{
            WiFi.begin((char*)wifi_ssid_);
        }

        while( WiFi.status() != WL_CONNECTED) {
            if(millis() - wifi_timeout > 30000) return false;
            #ifdef ESP8266
            yield();
            #endif
        }
        THINGER_DEBUG("NETWORK", "Connected to WiFi!");
        wifi_timeout = millis();
        THINGER_DEBUG("NETWORK", "Getting IP Address...");
        while (WiFi.localIP() == INADDR_NONE) {
            if(millis() - wifi_timeout > 30000) return false;
            #ifdef ESP8266
            yield();
            #endif
        }
        THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", WiFi.localIP());
        return true;
    }

public:

    void add_wifi(const char* ssid, const char* password=NULL)
    {
        wifi_ssid_ = ssid;
        wifi_password_ = password;
    }

protected:
    Client client_;
    const char* wifi_ssid_;
    const char* wifi_password_;
};

#define ThingerWifi ThingerWifiClient<WiFiClient>

#endif