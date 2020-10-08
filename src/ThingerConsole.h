#ifndef THINGER_CONSOLE_H
#define THINGER_CONSOLE_H

#include <Print.h>
#include "ThingerClient.h"

#define BUFFER_SIZE 128

class ThingerConsole : public Print{

public:
    ThingerConsole(ThingerClient& client): client_(client), resource_(client["$console"]), index_(0)
    {
        buffer_[BUFFER_SIZE] = 0;
        resource_ >> [this](pson& out){
            out = (const char*) buffer_;
        };
    }
    
    virtual ~ThingerConsole(){
    }
    
    virtual size_t write(uint8_t value){
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

    /**
     * Dummy being implementation just for Serial compatibility
     */
    void begin(unsigned long){
        // do nothing, just for Serial compatibility
    }

    /**
     * Boolean operator
     * @return true if the remote console is connected, false otherwise
     */
    operator bool(){
        return resource_.stream_enabled();
    }

private:
    ThingerClient& client_;
    thinger::thinger_resource& resource_;
    size_t index_;
    uint8_t buffer_[BUFFER_SIZE+1];
};

#endif
