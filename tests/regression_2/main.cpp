#define THINGER_MULTITASK
#define THINGER_FREE_RTOS_MULTITASK
#include "Arduino.h"
#include <iostream>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

#include "ThingerConsole.h"
class TestConsole:public ThingerConsole {
public:using ThingerConsole::ThingerConsole;
    size_t pending(){return tx_index_;}
};
int main(){
    Client wire;ThingerClient client(wire,"u","d","c");TestConsole console(client);
    // Activate the real console resource through its protocol request handler.
    thinger::thinger_message start;
    start.set_signal_flag(thinger::thinger_message::START_STREAM);
    start.set_stream_id(23);
    thinger::thinger_message response(start);
    client["$console"].handle_request(start,response);
    assert(client["$console"].stream_enabled());
    wire.on_write=[&]{
        std::thread other([&]{assert(console.write('x')==1);assert(console.write('y')==1);});
        other.join();
    };
    for(int i=0;i<128;++i)assert(console.write('a')==1);
    assert(console.pending()==2);
    console.flush();assert(console.pending()==0);
    assert(console.write('\n')==1);assert(console.pending()==0);
    client["$console"].disable_streaming();
    std::thread a([&]{for(int i=0;i<10000;++i)console.write('a');});
    std::thread b([&]{for(int i=0;i<10000;++i)console.write('b');});
    a.join();b.join();
    assert(console.pending()<128);console.flush();assert(console.pending()==0);
    std::cout<<"console concurrency regression passed\n";
}
