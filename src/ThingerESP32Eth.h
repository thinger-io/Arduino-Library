#ifndef THINGER_ESP32ETH_H
#define THINGER_ESP32ETH_H

// TODO ESP32 TROUGHT ETHERNET DOES NOT SUPPORT SSL/TLS CONNECTIONS
#define _DISABLE_TLS_

#ifdef THINGER_FREE_RTOS
#include "ThingerESP32FreeRTOS.h"
#endif

#include <ETH.h>
#include <ThingerClient.h>
#include <functional>

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
                case SYSTEM_EVENT_ETH_START:
                    THINGER_DEBUG("NETWORK", "ETH Started");
                    break;
                case SYSTEM_EVENT_ETH_CONNECTED:
                    THINGER_DEBUG("NETWORK", "ETH Connected");
                    break;
                case SYSTEM_EVENT_ETH_GOT_IP:
                    THINGER_DEBUG_VALUE("NETWORK", "ETH MAC: ", ETH.macAddress());
                    THINGER_DEBUG_VALUE("NETWORK", "ETH IP: ", ETH.localIP());
                    THINGER_DEBUG_VALUE("NETWORK", "ETH FullDuplex: ", ETH.fullDuplex());
                    THINGER_DEBUG_VALUE("NETWORK", "ETH LinkSpeed: ", ETH.linkSpeed());
                    break;
                case SYSTEM_EVENT_ETH_DISCONNECTED:
                    THINGER_DEBUG("NETWORK", "ETH Disconnected");
                    break;
                case SYSTEM_EVENT_ETH_STOP:
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

    virtual bool network_connected(){
        return initialized_ ? ETH.linkUp() : false;
    }

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

    virtual bool connect_network(){
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

    WiFiClient client_;
    bool initialized_       = false;
    const char* hostname_   = "esp32-thinger";
    const char* ip_         = nullptr;
    const char* gateway_    = nullptr;
    const char* subnet_     = nullptr;
    const char* dns1_       = nullptr;
    const char* dns2_       = nullptr;
      
};

#endif