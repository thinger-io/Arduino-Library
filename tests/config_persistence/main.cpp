#include "common.h"
void test_yield(){}
#define THINGER_ESP8266_H
class ThingerESP8266:public ThingerClient {
    static Client wire;
public:ThingerESP8266(const char*u,const char*d,const char*c):ThingerClient(wire,u,d,c){}
};
Client ThingerESP8266::wire;
#include "ThingerESP8266WebConfig.h"
class Config:public ThingerESP8266WebConfig{
public:
    Config():ThingerESP8266WebConfig("u","d","c"){}
    using ThingerESP8266WebConfig::save_configuration;
    using ThingerWebConfig<ThingerESP8266>::run_config_portal;
};
static int notifications=0;static bool portal_result=true;
void on_config(pson&){++notifications;}
void on_wifi(bool result){portal_result=result;}
int main(){
    const std::vector<uint8_t> original={11,22,33};
    pson config;config["user"]="new-user";config["device"]="device";config["credential"]="credential";
    Config device;
    for(int failure=0;failure<5;++failure){
        LittleFS=TestFS{};
        LittleFS.files[CONFIG_FILE]=original;
        if(failure==0)LittleFS.mount_ok=false;
        if(failure==1)LittleFS.open_ok=false;
        if(failure==2)LittleFS.budget=0;
        if(failure==3)LittleFS.budget=5;
        if(failure==4)LittleFS.rename_ok=false;
        assert(!device.save_configuration(config));
        assert(LittleFS.files[CONFIG_FILE]==original);
        assert(!LittleFS.exists(CONFIG_FILE ".tmp"));
    }
    LittleFS=TestFS{};LittleFS.files[CONFIG_FILE]=original;
    assert(device.save_configuration(config));
    assert(LittleFS.files[CONFIG_FILE]!=original && !LittleFS.exists(CONFIG_FILE ".tmp"));
    device.set_on_config_callback(on_config);device.set_on_wifi_config(on_wifi);
    LittleFS.budget=0;WiFiManager manager;
    assert(!device.run_config_portal(manager,true));
    assert(!portal_result && notifications==0);
    LittleFS.budget=100000;
    assert(device.run_config_portal(manager,true));
    assert(portal_result && notifications==1);
    std::cout<<"Configuration persistence regression passed\n";
}
