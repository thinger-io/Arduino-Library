#ifndef THINGER_OTA_H
#define THINGER_OTA_H

#define THINGER_DO_NOT_INIT_MEMORY_ALLOCATOR
#include "ThingerClient.h"

class ThingerOTA{

public:

    ThingerOTA(ThingerClient& client){

        client["$ota"]["options"] >> [&](pson& options){
            options["enabled"] = enabled_;
            options["block_size"] = block_size_;
            fill_options(options);
        };

        client["$ota"]["begin"] = [&](pson& in, pson& out){
            if(in.is_empty()){
                out["success"] = false;
                return;
            }

            const char* firmware   = in["firmware"];
            const char* version    = in["version"];
            size_t size            = in["size"];
            size_t compressed_size = in["compressed_size"];

            THINGER_DEBUG("OTA", "Received OTA request...");
            THINGER_DEBUG_VALUE("OTA", "Firmware: ", firmware);
            THINGER_DEBUG_VALUE("OTA", "Firmware Size: ", size);
            THINGER_DEBUG_VALUE("OTA", "Compressed Size: ", compressed_size);
            THINGER_DEBUG("OTA", "Initializing update...");

            bool init = begin_ota(firmware, version, size, compressed_size);

            THINGER_DEBUG_VALUE("OTA", "Init OK: ", init);
            out["success"] = init;

            if(init)
            {
                // TODO remove user task handle while upgrading OTA
                THINGER_DEBUG("OTA", "Waiting for firmware...");
            }
        };

        client["$ota"]["write"] = [&](pson& in, pson& out){
            if(!in.is_bytes()){
                THINGER_DEBUG("OTA", "invalid input type received!");
                out["success"] = false;
                return;
            }

            // get buffer
            size_t size = 0;
            const void * buffer = NULL;
            in.get_bytes(buffer, size);

            THINGER_DEBUG_VALUE("OTA", "Received OTA part (bytes): ", size);

            // write buffer
            auto start = millis();
            auto success = write_ota((const char*)buffer, size);
            auto stop = millis();

            THINGER_DEBUG_VALUE("OTA", "Wrote OK: ", success);
            THINGER_DEBUG_VALUE("OTA", "Elapsed time ms: ", stop-start);

            out["success"] = success;
        };

        client["$ota"]["end"] >> [&](pson& out){
            THINGER_DEBUG("OTA", "Finishing...");
            bool result = end_ota();
            THINGER_DEBUG_VALUE("OTA", "Update OK: ", result);
            out["success"] = result;
        };

        client["$ota"]["reboot"] = [&](){
            client.reboot();
        };
    
    }

    ~ThingerOTA(){

    }

public:

    void set_block_size(size_t size){
        block_size_ = size;
    }

    void set_enabled(bool enabled){
        enabled_ = enabled;
    }

    // methods to be implemented by underlying device
    virtual bool begin_ota(const char* firmware, const char* version, size_t bytes_size, size_t compressed_size) = 0;
    virtual bool write_ota(const char* buffer, size_t bytes) = 0;
    virtual size_t remaining() = 0;
    virtual bool end_ota() = 0;

protected:
    virtual void fill_options(pson& options) = 0;
    bool enabled_ = true;
    uint16_t block_size_ = 32768;

};

#endif