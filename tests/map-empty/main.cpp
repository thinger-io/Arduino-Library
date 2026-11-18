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

int main(){

    {
        Protocol p;p["sensors"]["temperature"]>>[](pson& out){out=21;};
        ::thinger::thinger_message req;req.set_stream_id(9);req.set_signal_flag(::thinger::thinger_message::START_STREAM);
        req.resources().add("sensors").add("temperature");req.get_data()=1000;p.frame(req);
        p.handle(0,true);int writes=p.writes;
        p.handle(5000,false);
        assert(p.writes==writes+1);
        p.stop_streams();
    }
}
