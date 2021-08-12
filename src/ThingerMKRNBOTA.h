#ifndef THINGER_MKRNBOTA_H
#define THINGER_MKRNBOTA_H

#include "ThingerOTA.h"
#include <NBFileUtils.h>
#include <SBU.cpp>

class ThingerMKRNBOTA : public ThingerOTA{

    constexpr static const char* UPDATE_FILE = "UPDATE.BIN";
    constexpr static const char* UPDATE_FILE_OK = "UPDATE.OK";

public:
    ThingerMKRNBOTA(ThingerClient& client) : 
        ThingerOTA(client)
    {
        // initialize a default block size for MKRNB
        set_block_size(256);
    }

    virtual ~ThingerMKRNBOTA(){

    }
    
private:
    size_t ota_remaining_ = 0;
    char md5_checksum_[33];

protected:

    void fill_options(pson& options) override{
        // set esp8266 platform
        options["platform"] = "mkrnb";

        // request a md5 checksum
        options["checksum"] = "md5";
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
        if(file_utils_.begin(false)){
            if(file_utils_.existFile(UPDATE_FILE)){
                THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE);
                file_utils_.deleteFile(UPDATE_FILE);
            }
            if(file_utils_.existFile(UPDATE_FILE_OK)){
                THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE_OK);
                file_utils_.deleteFile(UPDATE_FILE_OK);
            }
            ota_remaining_ = size;
            return true;
        }
        return false;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        auto write = file_utils_.downloadFile(UPDATE_FILE, (const char*) buffer, bytes, true);
        ota_remaining_ -= bytes;
        return write==bytes;
    }

    size_t remaining() override{
        return ota_remaining_;
    }

    bool end_ota(pson& state) override{
        return file_utils_.createFile(UPDATE_FILE_OK, "OK", 2) == 2;
    }

private:

    NBFileUtils file_utils_;
};

#endif