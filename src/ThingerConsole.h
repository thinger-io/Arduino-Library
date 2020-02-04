#ifndef THINGER_CONSOLE_H
#define THINGER_CONSOLE_H

#define THINGER_DO_NOT_INIT_MEMORY_ALLOCATOR
#include <Print.h>
#include "ThingerClient.h"

#define BUFFER_SIZE 128

class ThingerConsole : public Print{

public:
    ThingerConsole(ThingerClient& client);
    virtual ~ThingerConsole();
    virtual size_t write(uint8_t value);

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