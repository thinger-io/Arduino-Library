#define THINGER_OTA_MD5_VERIFICATION 0
#define THINGER_OTA_COMPRESSION 0
#include "common.h"
void test_yield(){}
#include "ThingerOTA.h"
class TestOTA:public ThingerOTA {
public:
    int begins=0,writes=0;size_t last_size=999;
    TestOTA(ThingerClient& client):ThingerOTA(client){}
    bool begin_ota(const char*,const char*,size_t,pson&,pson&)override{++begins;return true;}
    bool write_ota(uint8_t*,size_t bytes,pson&)override{++writes;last_size=bytes;return true;}
    void fill_options(pson&)override{}
    bool running()const{return running_;}
};
void begin_request(ThingerClient& client,thinger::thinger_message& out){
    thinger::thinger_message in;
    in.get_data()["firmware"]="firmware.bin";in.get_data()["version"]="test";
    in.get_data()["size"]=1024;in.get_data()["chunk_size"]=512;
    client["$ota"]["begin"].handle_request(in,out);
}

int main(){
    Client wire;ThingerClient client(wire,"u","d","c");TestOTA ota(client);
    ota.set_enabled(false);
    thinger::thinger_message options_in,options_out;
    client["$ota"]["options"].handle_request(options_in,options_out);
    assert(!bool(options_out.get_data()["enabled"]));
    thinger::thinger_message rejected;begin_request(client,rejected);
    assert(!bool(rejected.get_data()["success"]) && ota.begins==0 && !ota.running());
    assert(std::string((const char*)rejected.get_data()["error"])=="OTA is disabled");
    ota.set_enabled(true);
    thinger::thinger_message accepted;begin_request(client,accepted);
    assert(bool(accepted.get_data()["success"]) && ota.begins==1 && ota.running());
    thinger::thinger_message duplicate;begin_request(client,duplicate);
    assert(!bool(duplicate.get_data()["success"]) && ota.begins==1);
    std::cout<<"OTA enable guard regression passed\n";
}
