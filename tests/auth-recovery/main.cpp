#include "Arduino.h"
#include <iostream>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

#define THINGER_WEBCONFIG_AUTH_ATTEMPTS 2
#include "ThingerWebConfig.h"
class Device:public ThingerClient {
    static Client wire;
public:Device(const char*u,const char*d,const char*c):ThingerClient(wire,u,d,c){}
    bool connect_network() override{++connects;return true;}
    bool network_connected() override{return true;}
    int connects=0;
};
Client Device::wire;
class Config:public ThingerWebConfig<Device> {
public:
    Config(const char*u="",const char*d="",const char*c=""):ThingerWebConfig<Device>(u,d,c){}
    bool allow_save=true; std::string saved;bool clean_credentials() override{return true;}
    bool load_configuration(pson& p) override{
        if(saved.empty())return false;
        p["setting"]="preserved";p["user"]="user";p["device"]="device";p["credential"]=saved.c_str();return true;
    }
    bool save_configuration(pson& p) override{if(!allow_save)return false; assert(std::string((const char*)p["setting"])=="preserved");saved=(const char*)p["credential"];return true;}
    using ThingerWebConfig<Device>::connect_network;
    using ThingerWebConfig<Device>::network_connected;
    using ThingerWebConfig<Device>::credentials_rejected;
    void transport_failure(){thinger_state_listener(THINGER_AUTH_FAILED);}
};
static bool button=false;
static bool authorize(){return button;}
class Handshake: public ThingerClient {
public:
    explicit Handshake(Client& client):ThingerClient(client,"u","d","c"){}
    bool handshake(){return connect("u","d","c");}
    unsigned rejections=0;
    void credentials_rejected() override{++rejections;}
};
int main(){
    Client wire;Handshake session(wire);assert(!session.handshake() && !session.rejections);
    ::thinger::thinger_message rejection;rejection.set_signal_flag(::thinger::thinger_message::REQUEST_ERROR);
    uint8_t raw[32];::thinger::thinger_memory_encoder encoder(raw,sizeof(raw));encoder.encode(rejection);
    wire.input.push_back(::thinger::MESSAGE);wire.input.push_back(encoder.bytes_written());
    wire.input.insert(wire.input.end(),raw,raw+encoder.bytes_written());
    assert(!session.handshake() && session.rejections==1);
    Config c;c.saved="previous";assert(c.connect_network());
    c.credentials_rejected();assert(c.network_connected());
    c.set_credential_recovery(authorize,"fixture-only-key");
    c.transport_failure();assert(c.network_connected());
    c.credentials_rejected();assert(!c.network_connected());
    assert(c.connect_network() && c.saved=="previous");
    button=true;c.credentials_rejected();
    WiFiManager::submitted="corrected";assert(c.connect_network());
    assert(c.saved=="corrected" && WiFiManager::blank_secret && WiFiManager::dedicated_key);
    WiFiManager::submitted="another";c.allow_save=false;c.credentials_rejected();
    assert(!c.connect_network() && c.saved=="corrected");
    WiFiManager::submitted="";c.allow_save=true;c.credentials_rejected();
    assert(c.connect_network() && c.saved=="corrected");
    Config fixed("u","d","c");fixed.set_credential_recovery(authorize,"fixture-only-key");
    fixed.credentials_rejected();assert(fixed.network_connected());
}
