#pragma once
#include "Arduino.h"
#include <vector>
class Client {
public:
    std::vector<uint8_t> bytes;
    std::function<void()> on_write;
    bool succeed=true;
    virtual ~Client()=default;
    virtual size_t write(const uint8_t* data,size_t size){
        if(on_write){auto cb=on_write;on_write=nullptr;cb();}
        if(!succeed)return 0;
        bytes.insert(bytes.end(),data,data+size);return size;
    }
    int readBytes(char*,size_t){return 0;}
    int available(){return 0;}
    int connected(){return 1;}
    int connect(const char*,unsigned){return 1;}
    void stop(){}
};
