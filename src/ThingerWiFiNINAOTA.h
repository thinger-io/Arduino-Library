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

    bool end_ota(pson& state) override{
        // the WiFi module does not report write errors, and the bootloader (SNU) programs the file as it is on the
        // next boot, so read it back and verify it before applying it, as a corrupted firmware would not boot
        if(md5_checksum_.length()==32 && !verify_file()){
            reset_ota();
            THINGER_DEBUG("OTA", "The firmware stored in the WiFi module is corrupted");
            state["error"] = "The firmware stored in the WiFi module is corrupted";
            return false;
        }
        return true;
    }

private:

    bool verify_file(){
        MD5_CTX md5_ctx;
        MD5::MD5Init(&md5_ctx);
        // the WiFi module returns up to 255 bytes on every read
        uint8_t buffer[128];
        for(size_t offset=0; offset<firmware_size_; offset+=sizeof(buffer)){
            size_t size = firmware_size_ - offset < sizeof(buffer) ? firmware_size_ - offset : sizeof(buffer);
            WiFiStorage.read(UPDATE_FILE_NAME, offset, buffer, size);
            MD5::MD5Update(&md5_ctx, buffer, size);
        }
        return verify_checksum(md5_ctx, md5_checksum_);
    }

};

#endif