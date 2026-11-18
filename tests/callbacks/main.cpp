#define THINGER_MULTITASK
#define THINGER_FREE_RTOS_MULTITASK
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
    Protocol p;unsigned called=0;
    auto& resource=p["temperature"];
    resource >> [&](pson& out){assert(callback_depth);++called;out=21;};
    ::thinger::thinger_message req;req.set_stream_id(9);req.resources().add("temperature");p.frame(req);
    p.handle(0,true);assert(called==1 && !callback_depth);
    ::thinger::thinger_message start;start.set_stream_id(10);
    start.set_signal_flag(::thinger::thinger_message::START_STREAM);
    start.resources().add("temperature");start.get_data()=1000;p.frame(start);p.handle(0,true);
    unsigned before=called;p.handle(5000,false);assert(called==before+1 && !callback_depth);
    before=called;p.write_bucket("test",resource,false);assert(called==before+1 && !callback_depth);

    // A synchronous resource callback may stop its own transport.
    Client wire;
    ThingerClient client(wire,"u","d","c");
    client["pause"] >> [&](pson& out){
        assert(callback_depth);client.stop();out=true;
    };
    client.write_bucket("test", client["pause"], false);
    assert(wire.stops==1 && !callback_depth);
}
