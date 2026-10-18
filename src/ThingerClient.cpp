#include <Arduino.h>
#include "thinger/pson.h"

using namespace protoson;

#ifndef THINGER_DO_NOT_INIT_MEMORY_ALLOCATOR
    // THINGER_USE_STATIC__MEMORY (double underscore) is kept for backward compatibility
    #if !defined(THINGER_USE_STATIC_MEMORY) && !defined(THINGER_USE_STATIC__MEMORY)
        dynamic_memory_allocator alloc;
    #else
        #ifndef THINGER_STATIC_MEMORY_SIZE
            #define THINGER_STATIC_MEMORY_SIZE 512
        #endif
        circular_memory_allocator<THINGER_STATIC_MEMORY_SIZE> alloc;
    #endif
    memory_allocator& protoson::pool = alloc;
#endif