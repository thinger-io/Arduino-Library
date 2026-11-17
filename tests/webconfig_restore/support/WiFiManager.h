#pragma once
#include "Arduino.h"
#include <vector>
#define WL_CONNECTED 3
struct TestWiFi { int status(){return WL_CONNECTED;} };
static TestWiFi WiFi;
class WiFiManagerParameter {
    const char* id_; std::string value_; int length_;
public:
    WiFiManagerParameter(const char* id,const char*,const char* v,int n,const char* =""):id_(id),value_(v),length_(n){}
    const char* getValue(){return value_.c_str();}
    const char* getID(){return id_;}
    int getValueLength(){return length_;}
    void setValue(const char* v,int){value_=v;}
};
class WiFiManager {
public:
    static bool saved;
    std::vector<WiFiManagerParameter*> params;
    void setDebugOutput(bool){}
    void setConfigPortalTimeout(unsigned){}
    bool getWiFiIsSaved(){return saved;}
    void addParameter(WiFiManagerParameter* p){params.push_back(p);}
    bool startConfigPortal(const char*,const char*){return true;}
};
bool WiFiManager::saved=true;
