#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <vector>
#include "WiFi.h"

// Inject failure only into the allocations used for owned credentials (THINGER_WIFI_MALLOC), and
// track their lifetime without depending on private storage fields.
static bool fail_next_allocation=false;
static std::vector<void*> credential_allocations;
static void* test_malloc(size_t size){
    if(fail_next_allocation){ fail_next_allocation=false; return nullptr; }
    void* p=std::malloc(size);
    if(p) credential_allocations.push_back(p);
    return p;
}
static void test_free(void* p){
    auto it=std::find(credential_allocations.begin(), credential_allocations.end(), p);
    if(it!=credential_allocations.end()) credential_allocations.erase(it);
    std::free(p);
}
#define THINGER_WIFI_MALLOC test_malloc
#define THINGER_WIFI_FREE test_free

#include "ThingerWifi.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

class Device : public ThingerWifiClient<Client> {
public:
    Device():ThingerWifiClient<Client>("user", "device", "credential"){}
    using ThingerWifiClient<Client>::wifi_begin;
    const char* ssid() const { return wifi_ssid_; }
    const char* password() const { return wifi_password_; }
#ifdef THINGER_WIFI_MULTI
    WiFiMulti& multi(){ return wifi_multi_; }
    size_t networks() const { return wifi_networks_; }
#endif
#if defined(ESP32) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    const char* enterprise_ssid() const { return enterprise_ssid_; }
#endif
};

static void test_personal_credentials(){
    Device device;
    char ssid[]="original-network";
    char password[]="original-password";
    device.add_wifi(ssid, password);
    ssid[0]='X'; password[0]='X';
    assert(std::string(device.ssid())=="original-network");
    assert(std::string(device.password())=="original-password");
    assert(device.wifi_begin());
    assert(WiFi.ssid=="original-network" && WiFi.password=="original-password");

    Device temporary;
    {
        std::string s="temporary-network", p(64, 'p');
        temporary.add_wifi(s.c_str(), p.c_str());
        s.assign(4096, 'x'); p.assign(4096, 'y');
    }
    assert(temporary.wifi_begin());
    assert(WiFi.ssid=="temporary-network" && WiFi.password==std::string(64, 'p'));
    assert(temporary.wifi_begin()); // Reconnection still uses the owned values.

    Device open;
    open.add_wifi("open-network");
    assert(open.password()==nullptr);
    assert(open.wifi_begin() && WiFi.null_password);
    Device empty;
    empty.add_wifi("empty-password", "");
    assert(empty.password()!=nullptr && !*empty.password());

    Device failed;
    fail_next_allocation=true;
    failed.add_wifi("new-network", "new-password");
    assert(!fail_next_allocation);
    assert(failed.ssid()==nullptr && failed.password()==nullptr);
#ifdef THINGER_WIFI_MULTI
    assert(failed.networks()==0 && failed.multi().networks.empty());
    failed.multi().reject_next=true;
    failed.add_wifi("rejected-network", "password");
    // the first network is still connected directly even if WiFiMulti rejects it (as before 2.49.0)
    assert(std::string(failed.ssid())=="rejected-network" && failed.networks()==0);
    assert(failed.wifi_begin() && WiFi.ssid=="rejected-network");
    failed.add_wifi("retry-network", "password");
    assert(failed.wifi_begin() && failed.multi().runs==1);
    assert(failed.multi().networks[0].ssid=="retry-network");
    failed.multi().result=WL_NO_SSID_AVAIL;
    WiFi.ssid.clear();
    assert(failed.wifi_begin() && failed.multi().runs==2 && WiFi.ssid=="rejected-network");
    assert(std::string(failed.ssid())=="rejected-network" && failed.networks()==1);
#else
    failed.add_wifi("retry-network", "password");
    assert(std::string(failed.ssid())=="retry-network");
#endif

#ifdef THINGER_WIFI_MULTI
    device.add_wifi("second-network", "second-password");
    assert(device.networks()==2);
    assert(std::string(device.ssid())=="original-network");
    assert(device.multi().networks[1].ssid=="second-network");
    assert(device.wifi_begin() && device.multi().runs==1);
    device.multi().reject_next=true;
    device.add_wifi("rejected", "password");
    assert(device.networks()==2);
#else
    const char* old_ssid=device.ssid();
    fail_next_allocation=true;
    device.add_wifi("replacement", "replacement-password");
    assert(device.ssid()==old_ssid);
    assert(std::string(device.password())=="original-password");
    // Re-registering pointers to the current owned storage must also be safe.
    device.add_wifi(device.ssid(), device.password());
    assert(std::string(device.ssid())=="original-network");
    device.add_wifi("replacement", "replacement-password");
    assert(device.wifi_begin() && WiFi.ssid=="replacement");
    device.add_wifi(nullptr, nullptr);
    assert(device.ssid()==nullptr && device.password()==nullptr);
#endif
}

#if defined(ESP32) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
static void test_enterprise_credentials(){
    Device device;
    char ssid[]="enterprise", user[]="username", password[]="password";
    char identity[]="outer-identity", ca[]="test-ca-certificate";
    device.add_wifi_enterprise(ssid, user, password, identity, ca);
    ssid[0]='X'; user[0]='X'; password[0]='X'; identity[0]='X'; ca[0]='X';
    assert(device.wifi_begin());
    assert(WiFi.ssid=="enterprise" && WiFi.username=="username");
    assert(WiFi.password=="password" && WiFi.identity=="outer-identity");
    assert(WiFi.ca=="test-ca-certificate");

    for(const char* identity_default : {static_cast<const char*>(nullptr), ""}){
        Device temporary;
        {
            std::string s="temporary-enterprise", u="user", p="password", c(1024, 'c');
            temporary.add_wifi_enterprise(s.c_str(), u.c_str(), p.c_str(), identity_default, c.c_str());
            s.assign(4096, 'x'); u.assign(4096, 'x'); p.assign(4096, 'x'); c.assign(4096, 'x');
        }
        assert(temporary.wifi_begin());
        assert(WiFi.ssid=="temporary-enterprise" && WiFi.identity=="user");
        assert(WiFi.username=="user" && WiFi.password=="password");
        assert(WiFi.ca==std::string(1024, 'c'));
    }
    Device failed;
    failed.add_wifi("fallback", "fallback-password");
    fail_next_allocation=true;
    failed.add_wifi_enterprise("enterprise", "user", "password", "", "ca");
    assert(!fail_next_allocation && failed.enterprise_ssid()==nullptr);
    assert(failed.wifi_begin() && WiFi.ssid=="fallback");
    failed.add_wifi_enterprise("enterprise", "user", "password", "", "ca");
    assert(failed.wifi_begin() && WiFi.ssid=="enterprise");
    assert(WiFi.identity=="user");
}
#endif

int main(int argc, char** argv){
    const bool enterprise_only=argc>1 && std::string(argv[1])=="enterprise";
    if(!enterprise_only) test_personal_credentials();
#if defined(ESP32) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    test_enterprise_credentials();
#else
    assert(!enterprise_only);
#endif
    assert(credential_allocations.empty());
    std::cout << "WiFi credential ownership regression passed\n";
}
