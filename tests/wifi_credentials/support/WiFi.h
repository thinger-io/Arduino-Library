#pragma once
#include "Arduino.h"

enum { WL_CONNECTED=3, WL_NO_SSID_AVAIL=1, WL_CONNECT_FAILED=4 };
enum { WPA2_AUTH_PEAP=0, ARDUINO_EVENT_WIFI_STA_DISCONNECTED=0,
       WIFI_REASON_NO_AP_FOUND=1, WIFI_REASON_AUTH_FAIL=2,
       WIFI_REASON_AUTH_EXPIRE=3, WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT=4,
       WIFI_REASON_HANDSHAKE_TIMEOUT=5 };
#define INADDR_NONE 0xffffffffU
using IPAddress = uint32_t;
using arduino_event_id_t = int;
struct arduino_event_info_t { struct { uint16_t reason; } wifi_sta_disconnected; };
struct TestESP { void restart(){} };
static TestESP ESP;

struct TestWiFi {
    std::string ssid, password, username, identity, ca;
    bool null_password=false;
    int direct_calls=0;
    int status(){ return WL_CONNECTED; }
    IPAddress localIP(){ return 1; }
    std::string SSID(){ return ssid; }
    template<class Callback> void onEvent(Callback, int){}
    void disconnect(){}
    void begin(){}
    void begin(const char* s, const char* p){
        ++direct_calls;
        ssid = s ? s : "";
        password = p ? p : "";
        null_password = p == nullptr;
    }
    void begin(const char* s, int, const char* i, const char* u,
               const char* p, const char* certificate){
        begin(s, p);
        identity = i ? i : "";
        username = u ? u : "";
        ca = certificate ? certificate : "";
    }
};
static TestWiFi WiFi;
