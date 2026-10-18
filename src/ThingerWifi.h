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

#ifndef THINGER_WIFI_H
#define THINGER_WIFI_H

#include "ThingerClient.h"

// ESP32, ESP8266 and the Arduino-Pico core can keep several networks and connect to the strongest one
#if defined(ESP8266)
    #include <ESP8266WiFiMulti.h>
    #define THINGER_WIFI_MULTI ESP8266WiFiMulti
#elif defined(ESP32) || (defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED))
    #include <WiFiMulti.h>
    #define THINGER_WIFI_MULTI WiFiMulti
#endif

template <class Client>
class ThingerWifiClient : public ThingerClient {

public:
    ThingerWifiClient(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential)
    {}

    ~ThingerWifiClient(){

    }

protected:

    virtual bool network_connected(){
        return (WiFi.status() == WL_CONNECTED) && !(WiFi.localIP() == (IPAddress)INADDR_NONE);
    }

    virtual bool connect_network(){
        unsigned long wifi_timeout = millis();

#ifdef ESP32
        // ESP32 retries by itself, so WiFi.status() loses the reason of a failed connection: keep it
        if(!wifi_events_){
            WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info){
                // keep the known reasons, as a generic one usually follows them
                THINGER_ERROR error = wifi_reason_error(info.wifi_sta_disconnected.reason);
                if(error!=ERROR_UNKNOWN) wifi_disconnect_error() = error;
            }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
            wifi_events_ = true;
        }
        wifi_disconnect_error() = ERROR_UNKNOWN;
#endif

#if defined(ESP32) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
        // enterprise network that validates the authentication server (WiFiMulti cannot validate it)
        if(enterprise_ssid_!=nullptr){
            THINGER_DEBUG_VALUE("NETWORK", "Connecting to enterprise network ", enterprise_ssid_);
            WiFi.begin(enterprise_ssid_, WPA2_AUTH_PEAP, enterprise_identity_, enterprise_username_, enterprise_password_, enterprise_ca_);
            while(WiFi.status()!=WL_CONNECTED && millis()-wifi_timeout<30000) delay(100);
            if(WiFi.status()!=WL_CONNECTED){
                THINGER_DEBUG("NETWORK", "Cannot connect to the enterprise network");
                set_wifi_error();
                // try the other networks, if any
                if(wifi_networks_==0) return false;
                WiFi.disconnect();
                wifi_timeout = millis();
            }
        }
        if(enterprise_ssid_!=nullptr && WiFi.status()==WL_CONNECTED){
            THINGER_DEBUG_VALUE("NETWORK", "Connected to network ", WiFi.SSID());
        }else
#endif
#ifdef THINGER_WIFI_MULTI
        // several networks (or an enterprise one): scan and connect to the strongest known network
        if(wifi_networks_>1 || wifi_enterprise_){
            THINGER_DEBUG_VALUE("NETWORK", "Connecting to the strongest known network. Networks: ", wifi_networks_);
            if(wifi_multi_.run()!=WL_CONNECTED){
                THINGER_DEBUG("NETWORK", "Cannot connect to any known network");
                set_wifi_error();
                return false;
            }
            THINGER_DEBUG_VALUE("NETWORK", "Connected to network ", WiFi.SSID());
        }else
#endif
        if(wifi_ssid_!=nullptr){
            THINGER_DEBUG_VALUE("NETWORK", "Connecting to network ", wifi_ssid_);
            WiFi.begin((char*)wifi_ssid_, (char*) wifi_password_);
        }
        // espressif esp8266 and esp32 can connect to last network just with begin() (used with WiFiManager)
        #if defined(ESP8266) || defined(ESP32)
        else{
            THINGER_DEBUG("NETWORK", "Connecting to stored network");
            WiFi.begin();
        }
        #else
        else{
            THINGER_DEBUG("NETWORK", "Cannot connect to WiFi. SSID not set!");
            set_error(NETWORK_NOT_CONFIGURED);
            return false;
        }
        #endif

        while( WiFi.status() != WL_CONNECTED) {
            if(millis() - wifi_timeout > 30000){
                set_wifi_error();
                return false;
            }
            #ifdef ESP8266
            yield();
            #endif
        }

        THINGER_DEBUG("NETWORK", "Connected to WiFi!");
        wifi_timeout = millis();
        THINGER_DEBUG("NETWORK", "Getting IP Address...");
        while (WiFi.localIP() == (IPAddress)INADDR_NONE) {
            if(millis() - wifi_timeout > 30000){
                set_error(NETWORK_NO_IP);
                return false;
            }
            #ifdef ESP8266
            yield();
            #endif
        }
        THINGER_DEBUG_VALUE("NETWORK", "Got IP Address: ", WiFi.localIP());
        return true;
    }

    // reason of a failed connection, from the status reported by the WiFi library
    void set_wifi_error(){
#ifdef ESP32
        if(wifi_disconnect_error()!=ERROR_UNKNOWN){
            set_error(wifi_disconnect_error());
            return;
        }
#endif
        switch(WiFi.status()){
            case WL_NO_SSID_AVAIL:
                set_error(NETWORK_NOT_FOUND);
                break;
            // the network rejected the connection, usually because of a wrong password
            case WL_CONNECT_FAILED:
#ifdef ESP8266
            case WL_WRONG_PASSWORD:
#endif
                set_error(NETWORK_AUTH_ERROR);
                break;
            default:
                set_error(ERROR_UNKNOWN);
                break;
        }
    }

public:

    // ESP32, ESP8266 and Pico W accept several networks (call it once per network)
    void add_wifi(const char* ssid, const char* password=nullptr)
    {
#ifdef THINGER_WIFI_MULTI
        if(wifi_multi_.addAP(ssid, password)) wifi_networks_++;
        // a single network connects directly, without scanning, so it can be a hidden network
        if(wifi_ssid_!=nullptr) return;
#endif
        wifi_ssid_ = ssid;
        wifi_password_ = password;
    }

#if defined(ESP32) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    // WPA2-Enterprise network with PEAP/EAP-MSCHAPv2, i.e., eduroam. The (outer) identity is the username
    // if empty. ca_pem is the CA certificate of the authentication server: without it, the device does not
    // validate the server and could send the credentials to a fake access point with the same SSID
    void add_wifi_enterprise(const char* ssid, const char* username, const char* password, const char* identity="", const char* ca_pem=nullptr)
    {
        if(identity==nullptr || strlen(identity)==0) identity = username;
        // validating the server requires a direct connection (only one network can use it)
        if(ca_pem!=nullptr && enterprise_ssid_==nullptr){
            enterprise_ssid_ = ssid;
            enterprise_username_ = username;
            enterprise_password_ = password;
            enterprise_identity_ = identity;
            enterprise_ca_ = ca_pem;
            return;
        }
        if(wifi_multi_.addAP(ssid, password, username, identity)){
            wifi_networks_++;
            wifi_enterprise_ = true;
        }
    }
#endif

protected:
    Client client_;
    const char* wifi_ssid_ = nullptr;
    const char* wifi_password_ = nullptr;
#ifdef ESP32
    // error of the last disconnection reported by the ESP32 WiFi events
    static THINGER_ERROR& wifi_disconnect_error(){
        static THINGER_ERROR error = ERROR_UNKNOWN;
        return error;
    }

    static THINGER_ERROR wifi_reason_error(uint16_t reason){
        switch(reason){
            case WIFI_REASON_NO_AP_FOUND:
                return NETWORK_NOT_FOUND;
            case WIFI_REASON_AUTH_FAIL:
            case WIFI_REASON_AUTH_EXPIRE:
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            case WIFI_REASON_HANDSHAKE_TIMEOUT:
                return NETWORK_AUTH_ERROR;
            default:
                return ERROR_UNKNOWN;
        }
    }

    bool wifi_events_ = false;
#endif
#ifdef THINGER_WIFI_MULTI
    THINGER_WIFI_MULTI wifi_multi_;
    uint8_t wifi_networks_ = 0;
    bool wifi_enterprise_ = false;
#endif
#if defined(ESP32) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    const char* enterprise_ssid_ = nullptr;
    const char* enterprise_username_ = nullptr;
    const char* enterprise_password_ = nullptr;
    const char* enterprise_identity_ = nullptr;
    const char* enterprise_ca_ = nullptr;
#endif
};

#define ThingerWifi ThingerWifiClient<WiFiClient>

#endif