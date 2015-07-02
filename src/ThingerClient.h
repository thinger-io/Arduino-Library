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
            client_(client), username_(user), device_id_(device), device_password_(device_credential),
            temp_data_(NULL), out_size_(0)
    {

    }

    ~ThingerClient()
    {

    }

protected:

    virtual bool read(char* buffer, size_t size)
    {
        size_t total_read = 0;
        while(total_read<size){
            int read = client_.readBytes((char*)buffer, size-total_read);
            if(read<0) return false;
            total_read += read;
        }
        return total_read == size;
    }

    virtual bool write(const char* buffer, size_t size, bool flush=false){
        if(size>0){
            temp_data_ = (uint8_t*) realloc(temp_data_, out_size_ + size);
            memcpy(&temp_data_[out_size_], buffer, size);
            out_size_ += size;
        }
        if(flush){
            int write = client_.write(temp_data_, out_size_);
            free(temp_data_);
            temp_data_ = NULL;
            out_size_ = 0;
        }
    }

    virtual void disconnected(){
#ifdef _DEBUG_
    Serial.println("Disconnected by timeout!");
#endif
        client_.stop();
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

        if(!network){
            #ifdef _DEBUG_
            Serial.println("Network not connected! Connecting...");
            #endif
            network = connect_network();
            if(!network){
                #ifdef _DEBUG_
                Serial.println("Cannot connect to network!");
                #endif
                return false;
            }
            #ifdef _DEBUG_
            else{
                Serial.println("Network connected!");
            }
            #endif
        }

        bool client = client_.connected();
        if(!client){
            #ifdef _DEBUG_
            Serial.println("Client not connected!");
            #endif
            client = connect_client();
            if(!client){
            #ifdef _DEBUG_
            Serial.println("Cannot connect client!");
            #endif
                return false;
            }
            #ifdef _DEBUG_
            Serial.println("Client connected!");
            #endif
        }
        return network && client;
    }

    bool connect_client(){
        bool connected = false;
        #ifdef _DEBUG_
            Serial.print("Connecting to ");
            Serial.print(THINGER_SERVER);
            Serial.print(":");
            Serial.print(THINGER_PORT);
            Serial.println("...");
        #endif
        if (client_.connect(THINGER_SERVER, THINGER_PORT)) {
            #ifdef _DEBUG_
            Serial.println("Connected!");
            #endif
            #ifdef _DEBUG_
            Serial.println("Authenticating...");
            #endif
            connected = thinger::thinger::connect(username_, device_id_, device_password_);
            if(!connected){
                client_.stop();
                #ifdef _DEBUG_
                Serial.println("Cannot authenticate! Check your credentials");
                #endif
            }
            #ifdef _DEBUG_
            else{
                Serial.println("Authenticated!");
            }
            #endif
        }
        #ifdef _DEBUG_
        else{
            Serial.print("Cannot connect!");
        }
        #endif
        return connected;
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
    uint8_t * temp_data_;
    size_t out_size_;
};

#endif