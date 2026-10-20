// The MIT License (MIT)
//
// Copyright (c) 2026 THINK BIG LABS SL
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


#ifndef THINGER_ZEPHYR_H
#define THINGER_ZEPHYR_H

// Boards with WiFi on the Arduino Zephyr core (arduino:zephyr_main), i.e., Portenta H7, Opta or GIGA R1 WiFi
#include <WiFi.h>
#include <SocketWrapper.h>
#include <ZephyrSSLClient.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <errno.h>

// TLS credential tag used for the CA root certificate
#ifndef THINGER_ZEPHYR_CA_TAG
    #define THINGER_ZEPHYR_CA_TAG 0x5448
#endif

// TCP/TLS client on the Zephyr sockets. The TLS client of the core (ZephyrSSLClient 1.0.0) cannot connect: it sets
// a list of credential tags with uninitialized entries, and registers the credentials again on every connection
class ThingerZephyrClient : public arduino::Client {

public:

    void set_tls(bool tls, const char* root_ca){
        tls_ = tls;
        root_ca_ = root_ca;
    }

    int connect(IPAddress ip, uint16_t port) override{
        return connect(ip.toString().c_str(), port);
    }

    int connect(const char* host, uint16_t port) override{
        stop();
        struct addrinfo hints = {0};
        struct addrinfo* res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if(getaddrinfo(host, String(port).c_str(), &hints, &res)!=0 || res==nullptr) return 0;
        fd_ = zsock_socket(AF_INET, SOCK_STREAM, tls_ ? (int) IPPROTO_TLS_1_2 : (int) IPPROTO_TCP);
        bool ok = fd_>=0 && (!tls_ || configure_tls(host)) && zsock_connect(fd_, res->ai_addr, res->ai_addrlen)==0;
        freeaddrinfo(res);
        if(!ok){
            stop();
            return 0;
        }
        connected_ = true;
        return 1;
    }

    size_t write(uint8_t c) override{
        return write(&c, 1);
    }

    size_t write(const uint8_t* buffer, size_t size) override{
        size_t sent = 0;
        while(connected_ && sent<size){
            int ret = zsock_send(fd_, buffer + sent, size - sent, 0);
            if(ret<=0){
                connected_ = false;
                break;
            }
            sent += ret;
        }
        return sent;
    }

    int available() override{
        if(rx_pos_==rx_len_ && connected_){
            int ret = zsock_recv(fd_, rx_, sizeof(rx_), ZSOCK_MSG_DONTWAIT);
            if(ret>0){
                rx_pos_ = 0;
                rx_len_ = ret;
            }else if(ret==0 || (errno!=EAGAIN && errno!=EWOULDBLOCK)){
                // closed by the server, or connection error
                connected_ = false;
            }
        }
        return rx_len_ - rx_pos_;
    }

    int read() override{
        return available() ? rx_[rx_pos_++] : -1;
    }

    int read(uint8_t* buffer, size_t size) override{
        int count = available();
        if(count==0) return -1;
        if((size_t) count>size) count = size;
        memcpy(buffer, rx_ + rx_pos_, count);
        rx_pos_ += count;
        return count;
    }

    int peek() override{
        return available() ? rx_[rx_pos_] : -1;
    }

    void flush() override{

    }

    void stop() override{
        if(fd_>=0) zsock_close(fd_);
        fd_ = -1;
        connected_ = false;
        rx_pos_ = rx_len_ = 0;
    }

    uint8_t connected() override{
        return connected_ || rx_pos_<rx_len_;
    }

    operator bool() override{
        return connected();
    }

private:

    bool configure_tls(const char* host){
        // the core initializes mbedTLS when constructing its TLS client
        static ZephyrSSLClient mbedtls_init;
#ifdef THINGER_INSECURE_SSL
        int verify = TLS_PEER_VERIFY_NONE;
        return zsock_setsockopt(fd_, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify))==0 &&
               zsock_setsockopt(fd_, SOL_TLS, TLS_HOSTNAME, host, strlen(host))==0;
#else
        sec_tag_t tags[] = {ca_tag()};
        return register_ca() &&
               zsock_setsockopt(fd_, SOL_TLS, TLS_SEC_TAG_LIST, tags, sizeof(tags))==0 &&
               zsock_setsockopt(fd_, SOL_TLS, TLS_HOSTNAME, host, strlen(host))==0;
#endif
    }

    // credential tag of the registered CA root certificate
    static sec_tag_t& ca_tag(){
        static sec_tag_t tag = THINGER_ZEPHYR_CA_TAG;
        return tag;
    }

    // registers the CA root certificate (in DER format, smaller than PEM), once, as the credential keeps a reference
    // to it. The core cannot delete credentials, so a different certificate (set_root_ca()) uses the next tag
    bool register_ca(){
        static uint8_t der[2][2048];
        static const char* registered = nullptr;
        static uint8_t slot = 0;
        if(root_ca_==nullptr) return false;
        if(root_ca_==registered) return true;
        if(registered!=nullptr){
            ca_tag()++;
            slot ^= 1;
        }
        registered = nullptr;
        size_t size = pem_to_der(root_ca_, der[slot], sizeof(der[slot]));
        if(size==0 || tls_credential_add(ca_tag(), TLS_CREDENTIAL_CA_CERTIFICATE, der[slot], size)!=0) return false;
        registered = root_ca_;
        return true;
    }

    // decodes the base64 body of the first certificate of a PEM
    static size_t pem_to_der(const char* pem, uint8_t* der, size_t max){
        static const char header[] = "-----BEGIN CERTIFICATE-----";
        const char* begin = strstr(pem, header);
        if(begin==nullptr) return 0;
        begin += sizeof(header) - 1;
        const char* end = strstr(begin, "-----END CERTIFICATE-----");
        if(end==nullptr) return 0;
        uint32_t bits = 0;
        int count = 0;
        size_t size = 0;
        for(const char* c=begin; c<end; c++){
            int value;
            if(*c>='A' && *c<='Z') value = *c - 'A';
            else if(*c>='a' && *c<='z') value = *c - 'a' + 26;
            else if(*c>='0' && *c<='9') value = *c - '0' + 52;
            else if(*c=='+') value = 62;
            else if(*c=='/') value = 63;
            else continue;
            bits = (bits << 6) | value;
            count += 6;
            if(count>=8){
                count -= 8;
                if(size>=max) return 0;
                der[size++] = (bits >> count) & 0xFF;
            }
        }
        return size;
    }

    int fd_ = -1;
    bool tls_ = true;
    bool connected_ = false;
    const char* root_ca_ = nullptr;
    uint8_t rx_[512];
    size_t rx_pos_ = 0;
    size_t rx_len_ = 0;
};

#include "ThingerWifi.h"

class ThingerZephyr : public ThingerWifiClient<ThingerZephyrClient> {

public:
    ThingerZephyr(const char* user, const char* device, const char* device_credential) :
        ThingerWifiClient(user, device, device_credential)
    {}

    ~ThingerZephyr(){

    }

protected:

    bool connect_socket() override{
#ifdef _DISABLE_TLS_
        client_.set_tls(false, nullptr);
        return client_.connect(get_host(), THINGER_PORT);
#else
#ifdef THINGER_INSECURE_SSL
        THINGER_DEBUG("SSL/TLS", "Warning: TLS/SSL certificate will not be checked!")
#endif
        client_.set_tls(true, get_root_ca());
        return client_.connect(get_host(), THINGER_SSL_PORT);
#endif
    }

};

#endif
