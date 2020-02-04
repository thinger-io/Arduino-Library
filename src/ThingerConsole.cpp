#include "ThingerConsole.h"

ThingerConsole::ThingerConsole(ThingerClient& client) :
    client_(client),
    resource_(client["$"]["console"]),
    index_(0)
{
    buffer_[BUFFER_SIZE] = 0;
    resource_ >> [&](pson& out){
        out = (const char*) buffer_;
    };
}

ThingerConsole::~ThingerConsole(){

}

size_t ThingerConsole::write(uint8_t value){
    buffer_[index_++] = value;
    if(value=='\n' || index_==BUFFER_SIZE){
        buffer_[index_] = 0;
        index_=0;
        if(resource_.stream_enabled()){
            client_.stream(resource_);
        }
    }
    return 1;
}