// The MIT License (MIT)
//
// Copyright (c) 2015 THINGER LTD
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

#ifndef THINGERSMARTCONFIG_H
#define THINGERSMARTCONFIG_H

#include "ThingerClient.h"

class ThingerSmartConfig : public ThingerClient {

public:
    ThingerSmartConfig(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential)
    {
        pinMode(BUILTIN_LED, OUTPUT);
    }

    ~ThingerSmartConfig(){

    }

protected:

    virtual bool network_connected(){
        return WiFi.status() == WL_CONNECTED && !(WiFi.localIP() == INADDR_NONE);
    }

    virtual bool connect_network(){
        long wifi_timeout = millis();

        #ifdef _DEBUG_
        Serial.print(F("[NETWORK] Starting Smart Config... "));
            Serial.println(wifi_ssid_);
        #endif

        digitalWrite(BUILTIN_LED, LOW);

        WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
        while( WiFi.status() != WL_CONNECTED) {
            if(millis() - wifi_timeout > 15000) continue;
            yield();
        }

        if(WiFi.status() != WL_CONNECTED){
            wifi_timeout = millis();
            WiFi.beginSmartConfig();
            while(!WiFi.smartConfigDone()) {
                if(millis() - wifi_timeout > 60000){
                    WiFi.stopSmartConfig();
                    return false;
                }
                digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
                delay(500);
            }
        }

        #ifdef _DEBUG_
        Serial.println(F("[NETWORK] Connected to WiFi!"));
        #endif
        for(int i=0;i<10;i++){
            digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
            delay(100);
        }

        wifi_timeout = millis();
        #ifdef _DEBUG_
            Serial.println(F("[NETWORK] Getting IP Address..."));
        #endif
        while (WiFi.localIP() == INADDR_NONE) {
            if(millis() - wifi_timeout > 30000) return false;
            yield();
        }
        #ifdef _DEBUG_
            Serial.print(F("[NETWORK] Got IP Address: "));
            Serial.println(WiFi.localIP());
        #endif
        return true;
    }

private:

    WiFiClient client_;
};

#endif