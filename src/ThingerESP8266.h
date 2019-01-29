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
#include "ThingerWifi.h"

#ifndef _DISABLE_TLS_
class ThingerESP8266 : public ThingerWifiClient<WiFiClientSecure>{
#else
class ThingerESP8266 : public ThingerWifiClient<WiFiClient>{
#endif

public:
    ThingerESP8266(const char* user, const char* device, const char* device_credential) :
            ThingerWifiClient(user, device, device_credential)
    {

    }

    ~ThingerESP8266(){

    }

#ifndef _DISABLE_TLS_
protected:
    virtual bool connect_socket(){

    // since CORE 2.5.0, now it is used BearSSL by default
#ifndef _VALIDATE_SSL_CERTIFICATE_
        client_.setInsecure();
        THINGER_DEBUG("SSL/TLS", "Warning: use #define _VALIDATE_SSL_CERTIFICATE_ if certificate validation is required")
#else
        client_.setFingerprint(THINGER_TLS_FINGERPRINT);
        THINGER_DEBUG_VALUE("SSL/TLS", "SHA-1 certificate fingerprint: ", THINGER_TLS_FINGERPRINT)
#endif
        return client_.connect(get_host(), THINGER_SSL_PORT);
    }

    virtual bool secure_connection(){
        return true;
    }
#endif

};

#endif