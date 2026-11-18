#pragma once
#include "Arduino.h"
#include <vector>
#include <algorithm>
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
    std::vector<uint8_t> input;
    size_t offset=0;
    int readBytes(char* out,size_t requested){
        const size_t count=std::min(requested,input.size()-offset);
        if(count){std::memcpy(out,input.data()+offset,count);offset+=count;}
        return static_cast<int>(count);
    }
    int available(){return 0;}
    int connected(){return 1;}
    int connect(const char*,unsigned){return 1;}
    void stop(){}
};
