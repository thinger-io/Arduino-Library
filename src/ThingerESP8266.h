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

#ifndef THINGER_ESP8266_H
#define THINGER_ESP8266_H

#include <ESP8266WiFi.h>
#include <time.h>
#include "ThingerWifi.h"

#ifndef _DISABLE_TLS_
class ThingerESP8266 : public ThingerWifiClient<WiFiClientSecure>{
#else
class ThingerESP8266 : public ThingerWifiClient<WiFiClient>{
#endif

public:
    ThingerESP8266(const char* user, const char* device, const char* device_credential) :
            ThingerWifiClient(user, device, device_credential)
#ifndef _DISABLE_TLS_
#ifndef THINGER_INSECURE_SSL
        ,x509(get_root_ca())
#endif
#endif
    {
        
    }

    ~ThingerESP8266(){

    }

#ifndef _DISABLE_TLS_
protected:

#ifndef THINGER_INSECURE_SSL
    bool setClock() {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        THINGER_DEBUG("NTP_SYN", "Waiting for NTP time sync...");
        unsigned long ntp_timeout = millis();
        while (time(nullptr) < 8 * 3600 * 2) {
            if(millis() - ntp_timeout > 30000){
                THINGER_DEBUG("NTP_SYN", "Cannot sync time!");
                return false;
            }
            delay(500);
        }
        #ifdef _DEBUG_
        time_t now = time(nullptr);
        char* time = ctime(&now);
        time[strlen(time)-1]=0;
        THINGER_DEBUG_VALUE("NTP_SYN", "Current time (UTC): ", time);
        #endif
        return true;
    }

    bool connect_network() override{
        return ThingerWifiClient<WiFiClientSecure>::connect_network() && setClock();
    }
#endif

    bool connect_socket() override{

    // since CORE 2.5.0, now it is used BearSSL by default
#ifdef THINGER_INSECURE_SSL
        client_.setInsecure();
        THINGER_DEBUG("SSL/TLS", "Warning: TLS/SSL certificate will not be checked!")
#else
        client_.setTrustAnchors(&x509);
#endif
        return client_.connect(get_host(), THINGER_SSL_PORT);
    }

    bool secure_connection() override{
        return true;
    }

#ifndef THINGER_INSECURE_SSL
    private:
        BearSSL::X509List x509;
#endif

#endif

};

#endif