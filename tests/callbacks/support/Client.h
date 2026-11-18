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
    // incoming data, served one byte per call with a pause (a call returning 0) between bytes, as a slow link
    std::vector<uint8_t> incoming;size_t served=0;bool pause=false;
    int readBytes(char* data,size_t size){
        if(size==0||served>=incoming.size())return 0;
        pause=!pause;if(pause)return 0;
        data[0]=incoming[served++];return 1;
    }
    int available(){return 0;}
    int connected(){return 1;}
    int connect(const char*,unsigned){return 1;}
    unsigned stops=0;
    void stop(){++stops;}
};
