#include "common.h"
void test_yield(){}
#include "ThingerWebConfig.h"
class Device:public ThingerClient {
    static Client wire;
public:Device(const char*u,const char*d,const char*c):ThingerClient(wire,u,d,c){}
    bool connect_network()override{return false;}
};
Client Device::wire;
class Config:public ThingerWebConfig<Device>{
public:
    bool load=true,include_value=true;std::string stored;
    Config():ThingerWebConfig<Device>("u","d","c"){}
    bool clean_credentials()override{return true;}
    bool load_configuration(pson& p)override{
        if(!load)return false;
        if(include_value)p["site"]="installed-site";else p["site"]=42;
        return true;
    }
    bool save_configuration(pson& p)override{stored=(const char*)p["site"];return true;}
    using ThingerWebConfig<Device>::connect_network;
};
int main(){
    for(bool saved: {false,true}){
        WiFiManager::saved=saved;
        Config config;config.add_setup_parameter("site","Site","factory-default",32);
        if(saved){assert(!config.connect_network());assert(!config.connect_network());}
        assert(config.connect_network() && config.stored=="installed-site");
    }
    WiFiManager::saved=false;
    for(bool missing_file: {false,true}){
        Config config;config.load=!missing_file;config.include_value=false;
        config.add_setup_parameter("site","Site","factory-default",32);
        assert(config.connect_network() && config.stored=="factory-default");
    }
    std::cout<<"WebConfig parameter restoration regression passed\n";
}
