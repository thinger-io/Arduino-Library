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

#ifndef THINGER_CLIENT_H
#define THINGER_CLIENT_H

#include "thinger/thinger.h"

using namespace protoson;

dynamic_memory_allocator alloc;
//circular_memory_allocator<512> alloc;
memory_allocator& protoson::pool = alloc;

#ifndef THINGER_SERVER
    #define THINGER_SERVER "iot.thinger.io"
#endif

#ifndef THINGER_PORT
    #define THINGER_PORT 25200
#endif

#ifndef THINGER_SSL_PORT
    #define THINGER_SSL_PORT 25202
#endif

#ifndef THINGER_TLS_FINGERPRINT
    #define THINGER_TLS_FINGERPRINT "C3 90 0E 8B CB 2D 7A 32 1B 55 5C 00 FA 7B 39 5E 53 BC D2 8F"
#endif

#ifndef THINGER_TLS_HOST
    #define THINGER_TLS_HOST "thinger.io"
#endif

#define RECONNECTION_TIMEOUT 5000   // milliseconds
#define DEFAULT_READ_TIMEOUT 30000  // milliseconds

#ifdef _DEBUG_
    #define THINGER_DEBUG(type, text) Serial.print("["); Serial.print(F(type)); Serial.print("] "); Serial.println(F(text));
    #define THINGER_DEBUG_VALUE(type, text, value) Serial.print("["); Serial.print(F(type)); Serial.print("] "); Serial.print(F(text)); Serial.println(value);
#else
    #define THINGER_DEBUG(type, text) void();
    #define THINGER_DEBUG_VALUE(type, text, value) void();
#endif

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
        long start = millis();
        size_t total_read = 0;
        //THINGER_DEBUG_VALUE("THINGER", "Reading bytes: ", size);
        while(total_read<size){
            // For solving this issue: https://github.com/ntruchsess/arduino_uip/issues/149
            #ifdef UIPETHERNET_H
            int read = client_.read((uint8_t*)(buffer+total_read), size-total_read);
            #else
            int read = client_.readBytes((char*)buffer+total_read, size-total_read);
            #endif
            total_read += read;
            if(read<=0 && (!client_.connected() || abs(millis()-start)>=DEFAULT_READ_TIMEOUT)){
                #ifdef _DEBUG_
                THINGER_DEBUG("_SOCKET", "Cannot read from socket!");
                #endif
                return false;
            }
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

#ifdef _DEBUG_
            Serial.print(F(" ["));
            Serial.print(success ? F("OK") : F("FAIL"));
            Serial.println(F("]"));
            if(!success){
                THINGER_DEBUG_VALUE("THINGER", "Expected:", out_size_);
                THINGER_DEBUG_VALUE("THINGER", "Wrote:", written);
            }
#endif

            free(temp_data_);
            temp_data_ = NULL;
            out_size_ = 0;

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
        thinger::thinger::disconnected();
    }

    virtual bool connect_network(){
        return true;
    }

    virtual bool network_connected(){
        return true;
    }

    virtual bool connect_socket(){
        return client_.connect(THINGER_SERVER, THINGER_PORT);
    }

    virtual bool secure_connection(){
        return false;
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
        SOCKET_ERROR,
        THINGER_AUTHENTICATING,
        THINGER_AUTHENTICATED,
        THINGER_AUTH_FAILED
    };

    virtual void thinger_state_listener(THINGER_STATE state){
        #ifdef _DEBUG_
        switch(state){
            case NETWORK_CONNECTING:
                THINGER_DEBUG("NETWORK", "Starting connection...");
                break;
            case NETWORK_CONNECTED:
                THINGER_DEBUG("NETWORK", "Connected!");
                break;
            case NETWORK_CONNECT_ERROR:
                THINGER_DEBUG("NETWORK", "Cannot connect!");
                break;
            case SOCKET_CONNECTING:
                Serial.print(F("[_SOCKET] Connecting to "));
                Serial.print(THINGER_SERVER);
                Serial.print(F(":"));
                Serial.print(secure_connection() ? THINGER_SSL_PORT : THINGER_PORT);
                Serial.println(F("..."));
                Serial.print(F("[_SOCKET] Using secure TLS/SSL connection: "));
                Serial.println(secure_connection() ? F("yes") : F("no"));
                break;
            case SOCKET_CONNECTED:
                THINGER_DEBUG("_SOCKET", "Connected!");
                break;
            case SOCKET_CONNECTION_ERROR:
                THINGER_DEBUG("_SOCKET", "Error while connecting!");
                break;
            case SOCKET_DISCONNECTED:
                THINGER_DEBUG("_SOCKET", "Is now closed!");
                break;
            case SOCKET_TIMEOUT:
                THINGER_DEBUG("_SOCKET", "Timeout!");
                break;
            case THINGER_AUTHENTICATING:
                Serial.print(F("[THINGER] Authenticating. User: "));
                Serial.print(username_);
                Serial.print(F(" Device: "));
                Serial.println(device_id_);
                break;
            case THINGER_AUTHENTICATED:
                THINGER_DEBUG("THINGER", "Authenticated");
                break;
            case THINGER_AUTH_FAILED:
                THINGER_DEBUG("THINGER", "Auth Failed! Check username, device id, or device credentials.");
                break;
        }
        #endif
    }

    bool handle_connection()
    {
        // check if client is connected
        bool client = client_.connected();
        if(client) return true;

        // client is not connected, so check underlying network
        if(!network_connected()){
            thinger_state_listener(NETWORK_CONNECTING);
            if(!connect_network()){
                thinger_state_listener(NETWORK_CONNECT_ERROR);
                return false;
            }
            thinger_state_listener(NETWORK_CONNECTED);
        }

        // network is connected, so connect the client
        return connect_client();
    }

    bool connect_client(){
        bool connected = false;
        client_.stop(); // cleanup previous socket
        thinger_state_listener(SOCKET_CONNECTING);
        if (connect_socket()) {
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
            size_t available = client_.available();
            #ifdef _DEBUG_
            if(available>0){
                THINGER_DEBUG_VALUE("THINGER", "Available bytes: ", available);
            }
            #endif
            thinger::thinger::handle(millis(), available>0);
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

/**
 * Some syntactic sugar for defining input/output resources easily
 */

#if defined(__AVR__) || defined(ESP8266)

void digital_pin(protoson::pson& in, int pin){
    if(in.is_empty()){
        in = (bool) digitalRead(pin);
    }
    else{
        digitalWrite(pin, in ? HIGH : LOW);
    }
}

void inverted_digital_pin(protoson::pson& in, int pin){
    if(in.is_empty()){
        in = !(bool) digitalRead(pin);
    }
    else{
        digitalWrite(pin, in ? LOW : HIGH);
    }
}

#else

bool digital_pin(protoson::pson& in, int pin, bool& current_state){
    if(in.is_empty()) {
        in = current_state;
    }
    else{
        current_state = in;
        digitalWrite(pin, current_state ? HIGH : LOW);
    }
}

bool inverted_digital_pin(protoson::pson& in, int pin, bool& current_state){
    if(in.is_empty()) {
        in = !current_state;
    }
    else{
        current_state = in;
        digitalWrite(pin, current_state ? LOW : HIGH);
    }
}
#endif

/*
 * TODO ESP32 library does not implement analogWrite yet
 */

#ifndef ESP32
void analog_pin(protoson::pson& in, int pin){
    static int current = in;
    if(in.is_empty()){
        in = current;
    }
    else{
        current = in;
        analogWrite(pin, current);
    }
}
#endif

/**
 * AVR and ESP8266 supports reading the PIN state event if they are of output type. So they
 * can rely on reading the current pin state directly using the digitalRead function. However,
 * other devices cannot read the pin state while they are in output mode, so it is necessary to
 * keep a variable to keep the track of the current value.
 */
#if defined(__AVR__) || defined(ESP8266)
#define digitalPin(PIN) [](pson& in){ digital_pin(in, PIN); }
#define invertedDigitalPin(PIN) [](pson& in){ inverted_digital_pin(in, PIN); }
#else
#define digitalPin(PIN) [](pson& in){               \
    static bool state = LOW;                        \
    digital_pin(in, PIN, state);                    \
}
#define inverted_digital_pin(PIN) [](pson& in){     \
    static bool state = LOW;                        \
    inverted_digital_pin(in, PIN, state);           \
}
#endif

template <typename T>
inline bool inputResource(pson& in, T& value){
    if(in.is_empty()){
        in = value;
    } else{
        value = in;
        return true;
    }
    return false;
}

template<>
inline bool inputResource<String>(pson& in, String& value){
    if(in.is_empty()){
        in = value;
    } else{
        value = (const char*)in;
        return true;
    }
    return false;
}

#define analogPin(PIN) [](pson& in){ analog_pin(in, PIN);}
#define outputValue(value) [](pson& out){ out = value; }
#define outputString(value) [](pson& out){ out = value; }
#define servo(servo) [](pson& in){ if(in.is_empty()) in = (int)servo.read(); else servo.write((int)in); }
#define inputValue_1(value) [](pson& in){ inputResource(in, value); }
#define inputValue_2(value, callback) [](pson& in){ if(inputResource(in, value)){callback;}}
#define inputValue_X(x, value, callback, FUNC, ...)  FUNC
#define inputValue(...) inputValue_X(,##__VA_ARGS__,\
                                          inputValue_2(__VA_ARGS__),\
                                          inputValue_1(__VA_ARGS__)\
                                    )
#endif