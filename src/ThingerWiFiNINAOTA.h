#ifndef THINGER_WIFININAOTA_H
#define THINGER_WIFININAOTA_H

#define THINGER_OTA_MD5_VERIFICATION 1
#ifndef THINGER_OTA_CHUNK_SIZE
#define THINGER_OTA_CHUNK_SIZE 512
#endif

#include "ThingerOTA.h"
#include <WiFiNINA.h>
#include <SNU.h>

class ThingerWiFiNINAOTA : public ThingerOTA{

constexpr static const char * UPDATE_FILE_NAME      = "/fs/UPDATE.BIN";
constexpr static const char * UPDATE_FILE_NAME_LZSS = "/fs/UPDATE.BIN.LZSS";

public:
    ThingerWiFiNINAOTA(ThingerClient& client) : 
        ThingerOTA(client)
    {
        // initialize a default block size
        set_block_size(THINGER_OTA_CHUNK_SIZE);
    }

    virtual ~ThingerWiFiNINAOTA(){

    }

protected:

    void fill_options(pson& options) override{
        // set atmelsam platform
        options["platform"] = "atmelsam";

        // set lzss compression
        //options["compression"] = "lzss";
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
        reset_ota();
        return true;
    }

    bool reset_ota() override{
        // ensure no pending OTA files
        if(WiFiStorage.exists(UPDATE_FILE_NAME)){
            THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE_NAME);
            WiFiStorage.remove(UPDATE_FILE_NAME);
        }
        if(WiFiStorage.exists(UPDATE_FILE_NAME_LZSS)){
            THINGER_DEBUG_VALUE("OTA", "Removing: ", UPDATE_FILE_NAME_LZSS);
            WiFiStorage.remove(UPDATE_FILE_NAME_LZSS);
        }
        return true;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        WiFiStorage.write(UPDATE_FILE_NAME, firmware_offset_, buffer, bytes);
        return true;
    }

};

#endif