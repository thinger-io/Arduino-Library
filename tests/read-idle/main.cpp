#include "Arduino.h"
#include <iostream>
#include <deque>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

class Protocol:public ::thinger::thinger {
public:
    std::deque<char> incoming;int writes=0,disconnections=0;
    void push(const uint8_t* data,size_t size){for(size_t i=0;i<size;++i)incoming.push_back(data[i]);}
    void frame(::thinger::thinger_message& msg,size_t truncate=0){
        uint8_t bytes[1024];::thinger::thinger_memory_encoder e(bytes,sizeof(bytes));e.encode(msg);
        incoming.push_back(::thinger::MESSAGE);
        assert(e.bytes_written()<128);incoming.push_back(e.bytes_written());
        push(bytes,e.bytes_written()-truncate);
    }
    bool read(char* data,size_t size) override{
        if(incoming.size()<size)return false;
        for(size_t i=0;i<size;++i){data[i]=incoming.front();incoming.pop_front();}return true;
    }
    bool write(const char*,size_t,bool flush=false) override{if(flush)++writes;return true;}
    void disconnected() override{++disconnections;incoming.clear();::thinger::thinger::disconnected();}
};

class SlowClient:public ThingerClient {
public:
    SlowClient(Client& c):ThingerClient(c,"u","d","c"){}
    using ThingerClient::read;
};

int main(){

    {
        Client wire;for(int i=0;i<300;++i)wire.incoming.push_back(i);
        SlowClient client(wire);char buffer[300];
        assert(client.read(buffer,sizeof(buffer)));
        assert((uint8_t)buffer[299]==(uint8_t)299);
        assert(!client.read(buffer,1)); // No further data arrives.
    }
}
