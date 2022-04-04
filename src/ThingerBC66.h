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


#define GPRS_CONNECTION_TIMEOUT 30000

#include <LClient.h>
#include "ThingerClient.h"

class ThingerBC66 : public ThingerClient {

public:
    ThingerBC66(const char* user, const char* device, const char* device_credential) :
        ThingerClient(client_, user, device, device_credential)
    {
       
         
    }

    ~ThingerBC66(){

    }

protected:

    virtual bool network_connected(){
        return connected_;
    }

    virtual bool connect_network(){
        Dev.cereg(true);
        connected_ = true;
    }

    virtual bool secure_connection(){
        return false;
    }

protected:
    LClient client_;
    bool connected_ = false;

};

#endif