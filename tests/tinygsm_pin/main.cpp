#include "common.h"
void test_yield(){}
class Stream {};
// TinyGSM 0.12.0 enum values. The modem double rejects AT+CPIN on a ready SIM.
enum SimStatus { SIM_ERROR=0, SIM_READY=1, SIM_LOCKED=2, SIM_ANTITHEFT_LOCKED=3 };
class TinyGsm {
public:
    int unlocks=0, waits=0; SimStatus status=SIM_READY;
    TinyGsm(Stream&){}
    bool isNetworkConnected(){return false;}
    bool isGprsConnected(){return false;}
    bool restart(){return true;}
    SimStatus getSimStatus(){return status;}
    bool simUnlock(const char*){++unlocks; if(status!=SIM_LOCKED)return false; status=SIM_READY;return true;}
    bool waitForNetwork(unsigned){++waits; return status==SIM_READY;}
    bool gprsConnect(const char*,const char*,const char*){return true;}
};
class TinyGsmClient:public Client {public:TinyGsmClient(TinyGsm&){}};
class TinyGsmClientSecure:public TinyGsmClient {public:using TinyGsmClient::TinyGsmClient;};
#include "ThingerTinyGSM.h"
class Device:public ThingerTinyGSM {public:using ThingerTinyGSM::ThingerTinyGSM;using ThingerTinyGSM::connect_network;};
int main(){
    Stream serial; Device device("u","d","c",serial);
    device.setAPN("test.apn");
    assert(device.connect_network()); // Control: ready SIM with no PIN configured.
    device.setPIN("1234");
    bool result=device.connect_network();
    assert(result);
    assert(device.getTinyGsm().unlocks==0);

    device.getTinyGsm().status=SIM_LOCKED;
    assert(device.connect_network());
    assert(device.getTinyGsm().unlocks==1);
    for(auto state: {SIM_ERROR, SIM_ANTITHEFT_LOCKED}){
        device.getTinyGsm().status=state;
        assert(!device.connect_network());
        assert(device.getTinyGsm().unlocks==1);
    }
    std::cout<<"TinyGSM PIN regression passed\n";
}
