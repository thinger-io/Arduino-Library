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

#ifndef THINGER_CC3000_H
#define THINGER_CC3000_H

#include "ThingerClient.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

class ThingerCC3000 : public ThingerClient {

public:
    ThingerCC3000(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential),
            cc3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER)
    {
    }

    virtual ~ThingerCC3000(){
        cc3000.stop();
    }

protected:

    virtual bool network_connected(){
        return cc3000.checkConnected();
    }

    virtual bool connect_network(){
        if (!cc3000.begin())
        {
            while(1);
        }
        long wifi_timeout = millis();
        if (cc3000.connectToAP(wifi_ssid_, wifi_password_, WLAN_SEC_WPA2)) {
            while (!cc3000.checkDHCP())
            {
                if(millis() - wifi_timeout > 30000) return false;
            }
            return true;
        }else{
            cc3000.stop();
            return false;
        }
    }

public:

    void add_wifi(char* ssid, char* password)
    {
        wifi_ssid_ = ssid;
        wifi_password_ = password;
    }

private:
    Adafruit_CC3000 cc3000;
    Adafruit_CC3000_Client client_;
    char* wifi_ssid_;
    char* wifi_password_;
};

#endif