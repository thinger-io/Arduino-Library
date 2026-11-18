#pragma once
#include "WiFi.h"
#include <vector>

class WiFiMulti {
public:
    struct Network { std::string ssid, password; };
    std::vector<Network> networks;
    bool reject_next=false;
    int runs=0;
    int result=WL_CONNECTED;
    bool addAP(const char* s, const char* p, const char* =nullptr, const char* =nullptr){
        if(reject_next){ reject_next=false; return false; }
        if(!s || !*s) return false;
        networks.push_back({s, p ? p : ""});
        return true;
    }
    int run(){ ++runs; return result; }
};
