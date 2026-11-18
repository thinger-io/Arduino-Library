#include "Arduino.h"
#include "Client.h"
void test_yield(){}
class Stream {};
enum SimStatus { SIM_ERROR=0, SIM_READY=1, SIM_LOCKED=2, SIM_ANTITHEFT_LOCKED=3 };
class TinyGsm {
public:
    int restarts=0, waits=0, searching=0, needed=3; bool registered=false;
    TinyGsm(Stream&){}
    bool isNetworkConnected(){return registered;}
    bool isGprsConnected(){return registered;}
    // the network search restarts with the modem
    bool restart(){++restarts;searching=0;registered=false;return true;}
    String getModemInfo(){return "mock";}
    SimStatus getSimStatus(){return SIM_READY;}
    bool simUnlock(const char*){return true;}
    // registers after `needed` registration waits without a restart
    bool waitForNetwork(unsigned){++waits;registered=++searching>=needed;return registered;}
    bool gprsConnect(const char*,const char*,const char*){return true;}
};
class TinyGsmClient:public Client {public:TinyGsmClient(TinyGsm&){}};
class TinyGsmClientSecure:public TinyGsmClient {public:using TinyGsmClient::TinyGsmClient;};
#include "ThingerTinyGSM.h"
#include "common.h"
class Device:public ThingerTinyGSM {public:using ThingerTinyGSM::ThingerTinyGSM;using ThingerTinyGSM::connect_network;};
int main(){
    Stream serial;Device device("u","d","c",serial);
    device.setAPN("apn");
    int attempts=0;
    while(!device.connect_network()) assert(++attempts<20);
    assert(device.getTinyGsm().restarts==1 && attempts==2);
    // it restarts again after THINGER_TINYGSM_RESTART_ATTEMPTS failed registrations (i.e., the modem hangs)
    device.getTinyGsm().registered=false;device.getTinyGsm().needed=1000;
    for(int i=0;i<THINGER_TINYGSM_RESTART_ATTEMPTS;i++) assert(!device.connect_network());
    assert(device.getTinyGsm().restarts==1);
    assert(!device.connect_network());
    assert(device.getTinyGsm().restarts==2);
    std::cout<<"TinyGSM registration regression passed\n";
}
