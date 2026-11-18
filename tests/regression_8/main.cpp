#define THINGER_MULTITASK
#define THINGER_FREE_RTOS_MULTITASK
#include "Arduino.h"
#include <iostream>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

#include <deque>
class Protocol:public ::thinger::thinger {
public:
    std::deque<char> incoming;
    int writes=0;
    void frame(::thinger::thinger_message& msg){
        uint8_t bytes[1024];::thinger::thinger_memory_encoder e(bytes,sizeof(bytes));e.encode(msg);
        incoming.push_back(::thinger::MESSAGE);
        assert(e.bytes_written()<128);incoming.push_back(e.bytes_written());
        for(size_t i=0;i<e.bytes_written();++i)incoming.push_back(bytes[i]);
    }
    bool read(char* data,size_t size) override{
        if(incoming.size()<size)return false;
        for(size_t i=0;i<size;++i){data[i]=incoming.front();incoming.pop_front();}return true;
    }
    bool write(const char*,size_t,bool flush=false) override{if(flush)++writes;return true;}
};
int main(){
    Protocol p;p["ping"]>>[](pson& out){out=42;};
    srand(123);uint16_t ack_id=rand();srand(123);
    ::thinger::thinger_message req;req.set_stream_id(ack_id==9?10:9);req.resources().add("ping");p.frame(req);
    ::thinger::thinger_message ack;ack.set_stream_id(ack_id);ack.set_signal_flag(::thinger::thinger_message::REQUEST_OK);p.frame(ack);
    pson value;value=1;assert(p.set_property("test",value,true));assert(p.writes==2);
    // A missing acknowledgment returns false and releases ownership.
    assert(!p.set_property("test",value,true));
    std::atomic<bool> acquired{false};
    p.lock();p.lock();
    std::thread other([&]{p.lock();acquired=true;p.unlock();});
    std::this_thread::sleep_for(std::chrono::milliseconds(20));assert(!acquired);
    p.unlock();std::this_thread::sleep_for(std::chrono::milliseconds(20));assert(!acquired);
    p.unlock();other.join();assert(acquired);
    std::cout<<"recursive response regression passed\n";
}
