#pragma once
#include <vector>
struct {
    size_t limit=0;
    std::vector<uint8_t> bytes;
    bool begin(size_t size){limit=size;bytes.clear();return true;}
    size_t remaining(){return limit-bytes.size();}
    size_t write(uint8_t* data,size_t count){if(count>remaining())return 0;bytes.insert(bytes.end(),data,data+count);return count;}
    void abort(){}
    bool end(){return !remaining();}
    const char* errorString(){return "write error";}
    void setMD5(const char*){}
} Update;
