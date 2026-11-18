#include "Arduino.h"
#include <iostream>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

#include "ThingerWebConfig.h"
class Device:public ThingerClient {
    static Client wire;
public:Device(const char*u,const char*d,const char*c):ThingerClient(wire,u,d,c){}
    bool connect_network() override{return true;}
};
Client Device::wire;
class Config:public ThingerWebConfig<Device> {
public:
    Config(const char*u="",const char*d="",const char*c=""):ThingerWebConfig<Device>(u,d,c){}
    std::string saved_value;bool load=false;int saves=0;
    bool clean_credentials() override{return true;}
    bool load_configuration(pson& p) override{
        if(!load)return false;
        p["user"]=saved_value.c_str();p["device"]=saved_value.c_str();p["credential"]=saved_value.c_str();return true;
    }
    bool save_configuration(pson& p) override{++saves;saved_value=(const char*)p["credential"];return true;}
    using ThingerWebConfig<Device>::connect_network;
    using ThingerWebConfig<Device>::run_config_portal;
};
int main(){
    for(size_t n: {0u,1u,39u,40u,41u,200u}){
        std::string value(n,'q');Config c(value.c_str(),value.c_str(),value.c_str());
        assert(std::string(c.get_username())==(n<=40?value:""));
        assert(std::string(c.get_device_id())==(n<=40?value:""));
        c.set_user("old");c.set_device("old");c.set_credential("old");
        c.set_user(value.c_str());c.set_device(value.c_str());c.set_credential(value.c_str());
        assert(std::string(c.get_username())==(n<=40?value:"old"));
        assert(std::string(c.get_device_id())==(n<=40?value:"old"));
        WiFiManager manager;c.run_config_portal(manager,true);
        assert(c.saved_value==(n<=40?(n?value:WiFiManager::submitted):"old"));
        Config portal;WiFiManager::submitted=value;WiFiManager m;
        // empty credentials are not saved either (the portal would not open again to fix them)
        assert(portal.run_config_portal(m,true)==(n>0 && n<=40));
        assert(portal.saves==(n>0 && n<=40?1:0));
    }
    WiFiManager::submitted="portal";
    Config loaded;loaded.load=true;loaded.saved_value=std::string(40,'v');
    assert(loaded.connect_network());assert(std::string(loaded.get_username())==std::string(40,'v'));
    Config invalid;invalid.load=true;invalid.saved_value=std::string(100,'v');
    assert(invalid.connect_network());assert(std::string(invalid.get_username())=="portal");
    Config nulls(nullptr,nullptr,nullptr);assert(std::string(nulls.get_username()).empty());
    std::cout<<"credential bounds regression passed\n";
}
