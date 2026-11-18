#include "Arduino.h"
#include <vector>
#include <stdlib.h> // before the realloc macro: <stdlib.h> declares "using std::realloc"
static bool fail_next=false;
static void* test_realloc(void* ptr,size_t size){
    if(fail_next){fail_next=false;return nullptr;}
    return std::realloc(ptr,size);
}
#define realloc test_realloc
#include "Arduino.h"
#include <iostream>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool=allocator;

#undef realloc
class TestClient:public ThingerClient {
public: TestClient(Client& c):ThingerClient(c,"u","d","c"){}
    using ThingerClient::write;
};
int main(){
    Client wire; TestClient client(wire);
#ifndef THINGER_DISABLE_OUTPUT_BUFFER
    fail_next=true;
    assert(client.write("a",1));
    assert(client.write("b",1));
    assert(client.write(nullptr,0,true));
    assert(std::string(wire.bytes.begin(),wire.bytes.end())=="ab");
    wire.bytes.clear();
    assert(client.write("c",1));
    fail_next=true;
    std::string big(80,'d');
    assert(client.write(big.data(),big.size()));
    assert(client.write("e",1));
    assert(client.write(nullptr,0,true));
    assert(std::string(wire.bytes.begin(),wire.bytes.end())=="c"+big+"e");
    wire.succeed=false;fail_next=true;
    assert(!client.write("f",1));
    wire.succeed=true;
    assert(client.write("g",1));
    assert(client.write(nullptr,0,true));
    assert(client.write("z",1));
    assert(!client.write("x",SIZE_MAX));
    assert(client.write(nullptr,0,true));
#else
    assert(client.write("a",1));
    assert(wire.bytes.size()==1);
#endif
    std::cout<<"output allocation regression passed\n";
}
