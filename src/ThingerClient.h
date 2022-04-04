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

#include <Arduino.h>
#include <Client.h>
#include "thinger/thinger.h"

using namespace protoson;

#ifndef THINGER_SERVER
    #define THINGER_SERVER "iot.thinger.io"
    // CA root certificate from community servers
    // USERTrust RSA - Valid to January 18, 2038
    auto CA_ROOT_CERTIFICATE = R"(-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----)";
#else
    // Anty other private instance goes over LetsEncrypt
    // ISRG Root X1 - Valid To: June 4, 2035
    auto CA_ROOT_CERTIFICATE = R"(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----)";
#endif

#ifndef THINGER_PORT
    #define THINGER_PORT 25200
#endif

#ifndef THINGER_SSL_PORT
    #define THINGER_SSL_PORT 25202
#endif

#ifndef THINGER_TLS_HOST
    #define THINGER_TLS_HOST "thinger.io"
#endif

#ifndef RECONNECTION_TIMEOUT
    #define RECONNECTION_TIMEOUT 5000   // milliseconds
#endif

#ifndef DEFAULT_READ_TIMEOUT
    #define DEFAULT_READ_TIMEOUT 10000   // milliseconds
#endif

// set to 0 to increase buffer as required (less performing but memory saving!)
#ifndef THINGER_OUTPUT_BUFFER_GROWING_SIZE
    #define THINGER_OUTPUT_BUFFER_GROWING_SIZE 32
#endif


#if defined(_DEBUG_) || defined(THINGER_SERIAL_DEBUG)
    #define _DEBUG_
    #define THINGER_DEBUG(type, text) Serial.print("["); Serial.print(F(type)); Serial.print("] "); Serial.println(F(text));
    #define THINGER_DEBUG_VALUE(type, text, value) Serial.print("["); Serial.print(F(type)); Serial.print("] "); Serial.print(F(text)); Serial.println(value);
#else
    #define THINGER_DEBUG(type, text) void();
    #define THINGER_DEBUG_VALUE(type, text, value) void();
#endif

class ThingerClient : public thinger::thinger {

public:

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
        THINGER_AUTH_FAILED,
        THINGER_STOP_REQUEST
    };

    ThingerClient(Client& client, const char* user, const char* device, const char* device_credential) :
            client_(client),
            username_(user),
            device_id_(device),
            device_password_(device_credential),
            host_(THINGER_SERVER),
            root_ca_(CA_ROOT_CERTIFICATE)
#ifndef THINGER_DISABLE_OUTPUT_BUFFER
            ,out_buffer_(NULL), out_size_(0), out_total_size_(0)
#endif
    {
    }

    ~ThingerClient()
    {

    }

protected:

    virtual bool read(char* buffer, size_t size)
    {
        unsigned long start = millis();
        size_t total_read = 0;
        //THINGER_DEBUG_VALUE("THINGER", "Reading bytes: ", size);
        while(total_read<size){
            // For solving this issue: https://github.com/ntruchsess/arduino_uip/issues/149
            #ifdef UIPETHERNET_H
            int read = client_.read((uint8_t*)(buffer+total_read), size-total_read);
            #else
            int read = client_.readBytes((char*)buffer+total_read, size-total_read);
            #endif
            //THINGER_DEBUG_VALUE("THINGER", "Read bytes: ", read);
            total_read += read;

            /**
             * Check reading timeout
             */
            if(read<=0 && millis()-start>=DEFAULT_READ_TIMEOUT){
                #ifdef _DEBUG_
                THINGER_DEBUG("_SOCKET", "Cannot read from socket!");
                #endif
                return false;
            }

            /*
             * Without a small delays between readings, the MKRGSM1400 seems to miss information, i.e, reading a byte
             * after a byte. Maybe it is related to UART communication.
             */
#ifdef ARDUINO_SAMD_MKRGSM1400
            delay(2);
#endif

        }
        return total_read == size;
    }

    virtual bool write(const char* buffer, size_t size, bool flush=false){
        #ifndef THINGER_DISABLE_OUTPUT_BUFFER
        if(size>0){
            #ifdef _DEBUG_MEMORY_
            THINGER_DEBUG_VALUE("_MEMORY", "Writing to Output Buffer: ", size)
            #endif

            #if THINGER_OUTPUT_BUFFER_GROWING_SIZE > 0
            // check if it is necessary to increase output size
            if(out_size_+size>out_total_size_){
                // compute new buffer size
                out_total_size_ += (size/THINGER_OUTPUT_BUFFER_GROWING_SIZE + (size % THINGER_OUTPUT_BUFFER_GROWING_SIZE != 0)) * THINGER_OUTPUT_BUFFER_GROWING_SIZE;
                // reallocate buffer
                void * new_buffer = realloc(out_buffer_, out_total_size_);
                // increase current total size
                if(new_buffer!=NULL){
                    out_buffer_ = (uint8_t*) new_buffer;
                    #ifdef _DEBUG_MEMORY_
                    THINGER_DEBUG_VALUE("_MEMORY", "Increased buffer size to: ", out_total_size_)
                    THINGER_DEBUG_VALUE("_MEMORY", "Realloc Address: ", (unsigned long) out_buffer_)
                    #endif
                }else{
                    // Realloc problem! Not enough memory, flushing out buffer and writing directly from the incoming buffer
                    #ifdef _DEBUG_MEMORY_
                    THINGER_DEBUG("_MEMORY", "Output Memory Buffer Exhausted!");
                    #endif
                    return flush_out_buffer() && client_write(buffer, size);
                }
            }
            // copy current input to buffer
            memcpy(&out_buffer_[out_size_], buffer, size);
            out_size_ += size;

            #else
            void * new_buffer = realloc(out_buffer_, out_size_ + size);
            if(new_buffer!=NULL){
                #ifdef _DEBUG_MEMORY_
                THINGER_DEBUG_VALUE("_MEMORY", "Increased buffer size to: ", out_size_ + size)
                THINGER_DEBUG_VALUE("_MEMORY", "Realloc Address: ", (unsigned long) new_buffer)
                #endif
                out_buffer_ = (uint8_t*) new_buffer;
                memcpy(&out_buffer_[out_size_], buffer, size);
                out_size_ += size;
            }else{
                // Realloc problem! Not enough memory, flushing out buffer and writing directly from the incoming buffer
                #ifdef _DEBUG_MEMORY_
                THINGER_DEBUG("_MEMORY", "Output Memory Buffer Exhausted!");
                #endif
                return flush_out_buffer() && client_write(buffer, size);
            }
            #endif
        }
        if(flush){
            return flush_out_buffer();
        }
        return true;

        #else
        return client_write(buffer, size);
        #endif
    }

    bool client_write(const char* buffer, size_t size){
#ifdef _DEBUG_
        Serial.print(F("[THINGER] Writing bytes: "));
        Serial.print(size);
#endif

        size_t written = client_.write((uint8_t*) buffer, size);
        bool success = written == size;

#ifdef _DEBUG_
        Serial.print(F(" ["));
        Serial.print(success ? F("OK") : F("FAIL"));
        Serial.println(F("]"));
        if(!success){
            THINGER_DEBUG_VALUE("THINGER", "Expected:", size);
            THINGER_DEBUG_VALUE("THINGER", "Wrote:", written);
        }
#endif

        //FIXME Without this small delay or activating the debug (which takes time), the CC3200 does not work well. Why?
#ifdef __CC3200R1M1RGC__
        delay(1);
#endif

        return success;
    }

#ifndef THINGER_DISABLE_OUTPUT_BUFFER
    bool flush_out_buffer(){
        if(out_buffer_!=NULL && out_size_>0){
            bool success = client_write((const char*)out_buffer_, out_size_);

#ifdef _DEBUG_MEMORY_
            THINGER_DEBUG_VALUE("_MEMORY", "Releasing memory size: ", out_total_size_)
            THINGER_DEBUG_VALUE("_MEMORY", "Release Address: ", (unsigned long) out_buffer_)
#endif

            free(out_buffer_);
            out_buffer_ = NULL;
            out_size_ = 0;
            out_total_size_ = 0;
            return success;
        }
        return true;
    }
#endif

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
        th_synchronized(bool result=client_.connect(host_, secure_connection() ? THINGER_SSL_PORT : THINGER_PORT);)
        return result;
    }

    virtual bool secure_connection(){
#ifdef _DISABLE_TLS_
        return false;
#else
        return true;
#endif
    }

    virtual void run_reboot(){
        THINGER_DEBUG("THINGER", "Rebooting device...");
        #if defined(ARDUINO_ARCH_MEGAAVR)
        wdt_enable(WDT_PERIOD_8CLK_gc);
        #elif defined(__AVR__)
        wdt_enable(WDTO_15MS);
        #elif defined(__arm__)
        NVIC_SystemReset();
        #elif defined(ESP8266) || defined(ESP32)
        ESP.restart();
        #else
        #error "MCU reset procedure not implemented"
        #endif
        for (;;) {}
    }

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
                Serial.print(host_);
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
            case SOCKET_ERROR:
                THINGER_DEBUG("_SOCKET", "Socket Error");
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
            case THINGER_STOP_REQUEST:
                THINGER_DEBUG("THINGER", "Client was requested to stop.");
                break;
        }
        #endif
        if(state_listener_) state_listener_(state);
    }

    bool handle_connection()
    {
        // check if client is connected
        if(is_connected()) return true;

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

    void reboot(){
        delay(1000);
        stop();
        delay(1000);
        run_reboot();
    }

    void stop(){
        thinger_state_listener(THINGER_STOP_REQUEST);
        client_.stop();
        thinger_state_listener(SOCKET_DISCONNECTED);
        thinger::thinger::disconnected();
    }

    void handle(){
        if(handle_connection()){
            th_synchronized(size_t available = client_.available();)
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

    bool is_connected(){
        th_synchronized(bool result = client_.connected();)
        return result;
    }

    void set_credentials(const char* username, const char* device_id, const char* device_password){
        username_ = username;
        device_id_ = device_id;
        device_password_ = device_password;
    }

    void set_host(const char* host){
        host_ = host;
    }

    const char* get_host(){
        return host_;
    }

    const char* get_username(){
        return username_;
    }

    const char* get_device_id(){
        return device_id_;
    }

    void set_root_ca(const char* ca){
        root_ca_ = ca;
    }

    const char* get_root_ca(){
        return root_ca_;
    }

    void set_state_listener(std::function<void(THINGER_STATE)> state_listener){
        state_listener_ = state_listener;
    }

    Client& get_client(){
        return client_;
    }

private:

    Client& client_;
    const char* username_;
    const char* device_id_;
    const char* device_password_;
    const char* host_;
    const char* root_ca_;
    std::function<void(THINGER_STATE)> state_listener_;
#ifndef THINGER_DISABLE_OUTPUT_BUFFER
    uint8_t * out_buffer_;
    size_t out_size_;
    size_t out_total_size_;
#endif

};

/**
 * Some syntactic sugar for defining input/output resources easily
 */

#if defined(__AVR__) || defined(ESP8266)

inline void digital_pin(protoson::pson& in, int pin){
    if(in.is_empty()){
        in = (bool) digitalRead(pin);
    }
    else{
        digitalWrite(pin, in ? HIGH : LOW);
    }
}

inline void inverted_digital_pin(protoson::pson& in, int pin){
    if(in.is_empty()){
        in = !(bool) digitalRead(pin);
    }
    else{
        digitalWrite(pin, in ? LOW : HIGH);
    }
}

#else

inline void digital_pin(protoson::pson& in, int pin, bool& current_state){
    if(in.is_empty()) {
        in = current_state;
    }
    else{
        current_state = in;
        digitalWrite(pin, current_state ? HIGH : LOW);
    }
}

inline void inverted_digital_pin(protoson::pson& in, int pin, bool& current_state){
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
inline void analog_pin(protoson::pson& in, int pin){
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
