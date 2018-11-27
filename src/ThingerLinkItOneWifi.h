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

#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>

#include "ThingerClient.h"

class ThingerLinkItOneWifi : public ThingerClient {

public:
    ThingerLinkItOneWifi(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential)
    {}

    ~ThingerLinkItOneWifi(){

    }

protected:

    virtual bool network_connected(){
        return LWiFi.status() == LWIFI_STATUS_CONNECTED && !(LWiFi.localIP() == INADDR_NONE);
    }

    virtual bool connect_network(){
        unsigned long wifi_timeout = millis();
        THINGER_DEBUG_VALUE("NETWORK", "Connecting to network ", wifi_ssid_);
        LWiFi.begin();
        while(LWiFi.connect((char*)wifi_ssid_, LWiFiLoginInfo(LWIFI_WPA, wifi_password_))<=0) {
            if (millis() - wifi_timeout > 30000) return false;
            delay(100);
        }
        THINGER_DEBUG("NETWORK", "Connected to LWiFi!");
        THINGER_DEBUG("NETWORK", "Getting IP Address...");
        wifi_timeout = millis();
        while (LWiFi.localIP() == INADDR_NONE) {
            if(millis() - wifi_timeout > 30000) return false;
        }
        THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", LWiFi.localIP());
        return true;
    }

    virtual bool secure_connection(){
        return false;
    }

public:

    void add_wifi(const char* ssid, const char* password)
    {
        wifi_ssid_ = ssid;
        wifi_password_ = password;
    }

private:

    LWiFiClient client_;
    const char* wifi_ssid_;
    const char* wifi_password_;
};

#endif