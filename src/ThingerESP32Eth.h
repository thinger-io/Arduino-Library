#ifndef THINGER_ESP32ETH_H
#define THINGER_ESP32ETH_H

#ifdef THINGER_FREE_RTOS
#include "ThingerESP32FreeRTOS.h"
#endif

#include <ETH.h>
#include <ThingerClient.h>

#ifdef _DISABLE_TLS_
typedef WiFiClient ESP32Client;
#else
#include <WiFiClientSecure.h>
typedef WiFiClientSecure ESP32Client;
#endif

class ThingerESP32Eth : public ThingerClient

#ifdef THINGER_FREE_RTOS
,public ThingerESP32FreeRTOS
#endif

{
public:
    ThingerESP32Eth(const char* user, const char* device, const char* device_credential) :
            ThingerClient(client_, user, device, device_credential)
            #ifdef THINGER_FREE_RTOS
            ,ThingerESP32FreeRTOS(static_cast<ThingerClient&>(*this))
            #endif
    {

         WiFi.onEvent([](WiFiEvent_t event){
            switch (event) {
                case ARDUINO_EVENT_ETH_START:
                    THINGER_DEBUG("NETWORK", "ETH Started");
                    break;
                case ARDUINO_EVENT_ETH_CONNECTED:
                    THINGER_DEBUG("NETWORK", "ETH Connected");
                    break;
                case ARDUINO_EVENT_ETH_GOT_IP:
                    THINGER_DEBUG_VALUE("NETWORK", "ETH MAC: ", ETH.macAddress());
                    THINGER_DEBUG_VALUE("NETWORK", "ETH IP: ", ETH.localIP());
                    THINGER_DEBUG_VALUE("NETWORK", "ETH FullDuplex: ", ETH.fullDuplex());
                    THINGER_DEBUG_VALUE("NETWORK", "ETH LinkSpeed: ", ETH.linkSpeed());
                    break;
                case ARDUINO_EVENT_ETH_DISCONNECTED:
                    THINGER_DEBUG("NETWORK", "ETH Disconnected");
                    break;
                case ARDUINO_EVENT_ETH_STOP:
                    THINGER_DEBUG("NETWORK", "ETH Stopped");
                    break;
                default:
                    break;
            }
        });
    }
    
    virtual ~ThingerESP32Eth(){
    
    }

    void set_hostname(const char* hostname){
        hostname_ = hostname;
    }

    void set_address(const char* ip, const char* gateway, const char* subnet, const char* dns1="8.8.8.8", const char* dns2="8.8.4.4"){
        ip_ = ip;
        gateway_ = gateway;
        subnet_ = subnet;
        dns1_ = dns1;
        dns2_ = dns2;
    } 
    
protected:

    bool init_address(){
        if(ip_==nullptr) return true;
        bool result = true;
        IPAddress ip, gateway, subnet, dns1, dns2;
        result &= ip.fromString(ip_);
        result &= gateway.fromString(gateway_);
        result &= subnet.fromString(subnet_);
        result &= dns1.fromString(dns1_);
        result &= dns2.fromString(dns2_);
        return result && ETH.config(ip, gateway, subnet, dns1, dns2);
    }

    virtual bool network_connected() override{
        return initialized_ ? ETH.linkUp() : false;
    }

#ifndef _DISABLE_TLS_
    bool connect_socket() override{

#ifdef THINGER_INSECURE_SSL
        client_.setInsecure();
        THINGER_DEBUG("SSL/TLS", "Warning: TLS/SSL certificate will not be checked!")
#else
        client_.setCACert(get_root_ca());
#endif
        return client_.connect(get_host(), THINGER_SSL_PORT);
    }

    bool secure_connection() override{
        return true;
    }
#endif

    bool connect_network() override{
        if(!initialized_){
            initialized_ = ETH.begin();
            if(initialized_){
                ETH.setHostname(hostname_);
                if(!init_address()){
                    THINGER_DEBUG("NETWORK", "Cannot set provided IP configuration");
                }
            }
        }
        return network_connected();
    }

    ESP32Client client_;
    bool initialized_       = false;
    const char* hostname_   = "esp32-thinger";
    const char* ip_         = nullptr;
    const char* gateway_    = nullptr;
    const char* subnet_     = nullptr;
    const char* dns1_       = nullptr;
    const char* dns2_       = nullptr;
};

#endif