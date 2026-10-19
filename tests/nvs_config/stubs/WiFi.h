#pragma once
#include <string>
#include <vector>
#include <functional>
#include <stdint.h>
#include <cstring>
#include "sdkconfig.h"
using String = std::string;
extern int wifi_status;
extern bool native_wifi_saved;
enum { WL_CONNECTED = 3, WL_NO_SSID_AVAIL = 1, WL_CONNECT_FAILED = 4, WL_DISCONNECTED = 6 };
enum { WIFI_SCAN_RUNNING = -1, WIFI_AUTH_OPEN = 0, WIFI_AUTH_WPA2_PSK = 3, WIFI_AUTH_WPA2_ENTERPRISE = 5, WIFI_AUTH_MAX = 10 };
enum { ARDUINO_EVENT_WIFI_STA_DISCONNECTED, WIFI_REASON_NO_AP_FOUND, WIFI_REASON_AUTH_FAIL,
       WIFI_REASON_AUTH_EXPIRE, WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT, WIFI_REASON_HANDSHAKE_TIMEOUT, WPA2_AUTH_PEAP };
using arduino_event_id_t = int;
struct arduino_event_info_t { struct { uint16_t reason; } wifi_sta_disconnected; };
static const uint32_t INADDR_NONE = 0xffffffff;
struct IPAddress {
    uint32_t value;
    IPAddress(uint32_t v) : value(v) {}
    bool operator==(const IPAddress& b) const { return value == b.value; }
};
class WiFiClient {};
struct AvailableAP {
    std::string ssid, password;
    int32_t rssi;
    uint8_t auth;
    bool hidden;
    AvailableAP(const char* s, const char* p, int32_t r, uint8_t a, bool h = false)
        : ssid(s), password(p), rssi(r), auth(a), hidden(h) {}
};
extern unsigned long fake_millis;
inline unsigned long millis() { fake_millis += 100; return fake_millis; }
inline void delay(unsigned long ms) { fake_millis += ms; }
#define log_e(...) do {} while(0)
#define log_i(...) do {} while(0)
#define log_d(...) do {} while(0)
#define log_v(...) do {} while(0)
struct FakeWiFi {
    std::vector<AvailableAP> available;
    std::string selected, last_ssid, last_password, native_ssid, native_password;
    int scans = 0, begins = 0, default_begins = 0, disconnects = 0, erases = 0;
    uint8_t bssid[6] = {1, 2, 3, 4, 5, 6};
    int status() const { return wifi_status; }
    IPAddress localIP() const { return IPAddress(0x01020304); }
    template<class F> void onEvent(F, int) {}
    std::string SSID() const { return selected; }
    bool disconnect(bool = false, bool erase = false) {
        ++disconnects;
        if(erase) { ++erases; native_ssid.clear(); native_password.clear(); native_wifi_saved = false; }
        wifi_status = WL_DISCONNECTED; selected.clear(); return true;
    }
    int begin() { ++default_begins; return begin(native_ssid.c_str(), native_password.c_str()); }
    int begin(const char* ssid, const char* password = nullptr, int32_t = 0, const uint8_t* = nullptr, bool = true) {
        ++begins; last_ssid = ssid ? ssid : ""; last_password = password ? password : "";
        wifi_status = WL_NO_SSID_AVAIL;
        for(const auto& ap : available) {
            if(ap.ssid != last_ssid) continue;
            wifi_status = ap.password == last_password ? WL_CONNECTED : WL_CONNECT_FAILED;
            if(wifi_status == WL_CONNECTED) selected = last_ssid;
            break;
        }
        return wifi_status;
    }
    int begin(const char* ssid, int, const char*, const char*, const char* password,
              const char* = nullptr, const char* = nullptr, const char* = nullptr,
              int = -1, int32_t channel = 0, const uint8_t* address = nullptr) {
        return begin(ssid, password, channel, address);
    }
    int8_t scanNetworks(bool = false, bool = false) { ++scans; return static_cast<int8_t>(available.size()); }
    void getNetworkInfo(int i, String& ssid, uint8_t& auth, int32_t& rssi, uint8_t*& address, int32_t& channel) {
        const auto& ap = available[i]; ssid = ap.hidden ? "" : ap.ssid;
        auth = ap.auth; rssi = ap.rssi; address = bssid; channel = 1;
    }
    void scanDelete() {}
};
extern FakeWiFi WiFi;
