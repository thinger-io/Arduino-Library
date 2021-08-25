#ifndef THINGER_MKRNBOTA_H
#define THINGER_MKRNBOTA_H

#define THINGER_OTA_MD5_VERIFICATION 1

#include "ThingerOTA.h"
#include <NBFileUtils.h>
#include <SBU.h>

class ThingerMKRNBOTA : public ThingerOTA{

    constexpr static const char* UPDATE_FILE_COMPRESSED = "UPDATE.BIN.LZSS";
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

protected:

    void fill_options(pson& options) override{
        // set mkrnb platform
        options["platform"] = "mkrnb";

        // set lzss compression
        //options["compression"] = "lzss";
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
        if(file_utils_.begin(false)){
            reset_ota();
            return true;
        }
        return false;
    }


    bool reset_ota() override{
        if(file_utils_.existFile(UPDATE_FILE)){
            THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE);
            file_utils_.deleteFile(UPDATE_FILE);
        }
        if(file_utils_.existFile(UPDATE_FILE_OK)){
            THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE_OK);
            file_utils_.deleteFile(UPDATE_FILE_OK);
        }
        if(file_utils_.existFile(UPDATE_FILE_COMPRESSED)){
            THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE_COMPRESSED);
            file_utils_.deleteFile(UPDATE_FILE_COMPRESSED);
        }
        return true;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        return file_utils_.downloadFile(UPDATE_FILE, (const char*) buffer, bytes, true) == bytes;
    }

    bool end_ota(pson& state) override{
        return file_utils_.createFile(UPDATE_FILE_OK, "OK", 2) == 2;
    }

private:
    NBFileUtils file_utils_;
};

#endif