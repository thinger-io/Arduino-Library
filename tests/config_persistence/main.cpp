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
    using ThingerESP8266WebConfig::load_configuration;
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
        // a file system that cannot be mounted is not formatted, and the default config is restored
        assert(!LittleFS.formatted_on_begin && LittleFS.auto_format);
    }
    LittleFS=TestFS{};LittleFS.auto_format=false;
    assert(device.save_configuration(config) && !LittleFS.auto_format && !LittleFS.mounted);
    // the file system mounted by the sketch stays mounted
    LittleFS=TestFS{};LittleFS.begin();
    assert(device.save_configuration(config) && LittleFS.mounted && LittleFS.ends==0);
    pson loaded;
    device.load_configuration(loaded); // the mock File does not read back, only the mount state is checked
    assert(LittleFS.mounted && LittleFS.ends==0);
    assert(device.clean_credentials() && !LittleFS.exists(CONFIG_FILE));
    assert(LittleFS.mounted && LittleFS.ends==0);
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
