#pragma once
#include <string>
#include <vector>
#include "WiFi.h"
extern int portal_calls;
extern int wifi_status;
extern const char* submitted_user;
extern const char* submitted_device;
extern const char* submitted_credential;
class WiFiManagerParameter {
public:
    WiFiManagerParameter(const char* id, const char*, const char* value, int, const char* = "") : id_(id), value_(value) {}
    const char* getID() const { return id_.c_str(); }
    const char* getValue() const { return value_.c_str(); }
    std::string id_, value_;
};
class WiFiManager {
public:
    void setDebugOutput(bool) {}
    bool getWiFiIsSaved() { return native_wifi_saved; }
    String getWiFiSSID(bool) { return WiFi.native_ssid; }
    String getWiFiPass(bool) { return WiFi.native_password; }
    void addParameter(WiFiManagerParameter* p) { parameters_.push_back(p); }
    bool startConfigPortal(const char*, const char*) {
        ++portal_calls;
        for(auto p : parameters_) {
            if(p->id_ == "user") p->value_ = submitted_user;
            if(p->id_ == "device") p->value_ = submitted_device;
            if(p->id_ == "credential") p->value_ = submitted_credential;
        }
        wifi_status = 3;
        native_wifi_saved = true;
        WiFi.native_ssid = "portal-wifi";
        WiFi.native_password = "portal-password";
        WiFi.selected = WiFi.native_ssid;
        return true;
    }
private:
    std::vector<WiFiManagerParameter*> parameters_;
};
