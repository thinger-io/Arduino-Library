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
    for(int type=0;type<4;++type){
        Client wire;ThingerClient client(wire,"u","d","c");TestOTA ota(client);
        thinger::thinger_message accepted;begin_request(client,accepted);
        thinger::thinger_message in,out;
        if(type==0) in.get_data()="not binary";
        if(type==1) in.get_data()=42;
        if(type==2) in.get_data()["field"]="object";
        // type 3 leaves the request empty.
        client["$ota"]["write"].handle_request(in,out);
        assert(!bool(out.get_data()["success"]) && !ota.running() && ota.writes==0);
        assert(std::string((const char*)out.get_data()["error"])=="Invalid input type received");
        thinger::thinger_message late,late_out;
        uint8_t bytes[]={1,2,3};late.get_data().set_bytes(bytes,sizeof(bytes));
        client["$ota"]["write"].handle_request(late,late_out);
        assert(!bool(late_out.get_data()["success"]) && ota.writes==0);
    }
    Client wire;ThingerClient client(wire,"u","d","c");TestOTA ota(client);
    thinger::thinger_message accepted;begin_request(client,accepted);
    thinger::thinger_message valid,response;uint8_t bytes[]={1,2,3};
    valid.get_data().set_bytes(bytes,sizeof(bytes));
    client["$ota"]["write"].handle_request(valid,response);
    assert(bool(response.get_data()["success"]) && ota.running() && ota.writes==1 && ota.last_size==3);
    std::cout<<"OTA payload validation regression passed\n";
}
