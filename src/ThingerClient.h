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

#ifndef THINGER_CLIENT_H
#define THINGER_CLIENT_H

#include "thinger/thinger.h"

using namespace protoson;

dynamic_memory_allocator alloc;
memory_allocator& protoson::pool = alloc;

#define THINGER_SERVER "iot.thinger.io"
#define THINGER_PORT 25200

class ThingerClient : public thinger::thinger {

public:
    ThingerClient(Client& client, const char* user, const char* device, const char* device_credential) :
            client_(client), username_(user), device_id_(device), device_password_(device_credential)
    {

    }

    ~ThingerClient()
    {

    }

protected:

    virtual bool read(char* buffer, size_t size)
    {
        return client_.readBytes((char*)buffer, size) == size;
    }

    virtual bool write(const char* buffer, size_t size, bool flush=false){
        return client_.write((const uint8_t*) buffer, size) == size;
    }

    virtual bool connect_network(){
        return true;
    }

    virtual bool network_connected(){
        return true;
    }

    bool handle_connection()
    {
        bool network = network_connected();
        bool client = network && client_.connected();
        if(!network) network = connect_network();
        if(network && !client) client = connect_client();
        return network && client;
    }

    bool connect_client(){
        if (client_.connect(THINGER_SERVER, THINGER_PORT)) {
            return thinger::thinger::connect(username_, device_id_, device_password_);
        }
        return false;
    }

public:

    void handle(){
        if(handle_connection()){
            thinger::thinger::handle(millis(), client_.available()>0);
        }
    }

private:

    Client& client_;
    const char* username_;
    const char* device_id_;
    const char* device_password_;
};

#endif