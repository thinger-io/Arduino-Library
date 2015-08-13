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
//circular_memory_allocator<512> alloc;
memory_allocator& protoson::pool = alloc;

#define THINGER_SERVER "iot.thinger.io"
#define THINGER_PORT 25200
#define RECONNECTION_TIMEOUT 5000 // milliseconds

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

    // TODO Allow removing this Nagle's algorithm implementation if the underlying device already implements it
    virtual bool write(const char* buffer, size_t size, bool flush=false){
        if(size>0){
            temp_data_ = (uint8_t*) realloc(temp_data_, out_size_ + size);
            memcpy(&temp_data_[out_size_], buffer, size);
            out_size_ += size;
        }
        if(flush && out_size_>0){
            #ifdef _DEBUG_
            Serial.print(F("[THINGER] Writing bytes: "));
            Serial.print(out_size_);
            #endif

            size_t written = client_.write(temp_data_, out_size_);
            bool success = written == out_size_;
            free(temp_data_);
            temp_data_ = NULL;
            out_size_ = 0;

            #ifdef _DEBUG_
            Serial.print(F(" ["));
            Serial.print(success ? F("OK") : F("FAIL"));
            Serial.println(F("]"));
            #endif

            //FIXME Without this small delay or activating the debug (which takes time), the CC3200 does not work well. Why?
            #ifdef __CC3200R1M1RGC__
            delay(1);
            #endif

            return success;
        }
        return true;
    }

    virtual void disconnected(){
        thinger_state_listener(SOCKET_TIMEOUT);
        client_.stop();
        thinger_state_listener(SOCKET_DISCONNECTED);
    }

    virtual bool connect_network(){
        return true;
    }

    virtual bool network_connected(){
        return true;
    }

    enum THINGER_STATE{
        NETWORK_CONNECTING,
        NETWORK_CONNECTED,
        NETWORK_CONNECT_ERROR,
        SOCKET_CONNECTING,
        SOCKET_CONNECTED,
        SOCKET_CONNECTION_ERROR,
        SOCKET_DISCONNECTED,
        SOCKET_TIMEOUT,
        THINGER_AUTHENTICATING,
        THINGER_AUTHENTICATED,
        THINGER_AUTH_FAILED
    };

    virtual void thinger_state_listener(THINGER_STATE state){
        #ifdef _DEBUG_
        switch(state){
            case NETWORK_CONNECTING:
                Serial.println(F("[NETWORK] Starting connection..."));
                break;
            case NETWORK_CONNECTED:
                Serial.println(F("[NETWORK] Connected!"));
                break;
            case NETWORK_CONNECT_ERROR:
                Serial.println(F("[NETWORK] Cannot connect!"));
                break;
            case SOCKET_CONNECTING:
                Serial.print(F("[_SOCKET] Connecting to "));
                Serial.print(THINGER_SERVER);
                Serial.print(F(":"));
                Serial.print(THINGER_PORT);
                Serial.println(F("..."));
                break;
            case SOCKET_CONNECTED:
                Serial.println(F("[_SOCKET] Connected!"));
                break;
            case SOCKET_CONNECTION_ERROR:
                Serial.println(F("[_SOCKET] Error while connecting!"));
                break;
            case SOCKET_DISCONNECTED:
                Serial.println(F("[_SOCKET] Is now closed!"));
                break;
            case SOCKET_TIMEOUT:
                Serial.println(F("[_SOCKET] Timeout!"));
                break;
            case THINGER_AUTHENTICATING:
                Serial.print(F("[THINGER] Authenticating. User: "));
                Serial.print(username_);
                Serial.print(F(" Device: "));
                Serial.println(device_id_);
                break;
            case THINGER_AUTHENTICATED:
                Serial.println(F("[THINGER] Authenticated!"));
                break;
            case THINGER_AUTH_FAILED:
                Serial.println(F("[THINGER] Auth Failed! Check username, device id, or device credentials."));
                break;
        }
        #endif
    }

    bool handle_connection()
    {
        bool network = network_connected();

        if(!network){
            thinger_state_listener(NETWORK_CONNECTING);
            network = connect_network();
            if(!network){
                thinger_state_listener(NETWORK_CONNECT_ERROR);
                return false;
            }
            thinger_state_listener(NETWORK_CONNECTED);
        }

        bool client = client_.connected();
        if(!client){
            client = connect_client();
            if(!client){
                return false;
            }
        }
        return network && client;
    }

    bool connect_client(){
        bool connected = false;
        client_.stop(); // cleanup previous socket
        thinger_state_listener(SOCKET_CONNECTING);
        if (client_.connect(THINGER_SERVER, THINGER_PORT)) {
            thinger_state_listener(SOCKET_CONNECTED);
            thinger_state_listener(THINGER_AUTHENTICATING);
            connected = thinger::thinger::connect(username_, device_id_, device_password_);
            if(!connected){
                thinger_state_listener(THINGER_AUTH_FAILED);
                client_.stop();
                thinger_state_listener(SOCKET_DISCONNECTED);
            }
            else{
                thinger_state_listener(THINGER_AUTHENTICATED);
            }
        }
        else{
            thinger_state_listener(SOCKET_CONNECTION_ERROR);
        }
        return connected;
    }

public:

    void handle(){
        if(handle_connection()){
            #ifdef _DEBUG_
            if(client_.available()>0){
                Serial.print(F("[THINGER] Available bytes: "));
                Serial.println(client_.available());
            }
            #endif
            thinger::thinger::handle(millis(), client_.available()>0);
        }else{
            delay(RECONNECTION_TIMEOUT); // get some delay for a connection retry
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