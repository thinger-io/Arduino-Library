#ifndef THINGER_MBEDOTA_H
#define THINGER_MBEDOTA_H

#define THINGER_OTA_MD5_VERIFICATION 1
#ifndef THINGER_OTA_CHUNK_SIZE
#define THINGER_OTA_CHUNK_SIZE 2048
#endif

#include "ThingerOTA.h"
#include <mbed.h>
#include <SFU.h>
#include <FlashIAPBlockDevice.h>
#include <FATFileSystem.h>

#define SD_MOUNT_PATH           "ota"
#define FULL_UPDATE_FILE_PATH   "/" SD_MOUNT_PATH "/" "UPDATE.BIN"
#define FULL_UPDATE_FILE_PATH_COMPRESSED   FULL_UPDATE_FILE_PATH ".LZSS"

class ThingerMbedOTA : public ThingerOTA{

public:
    ThingerMbedOTA(ThingerClient& client) : 
        ThingerOTA(client),
        fs_(SD_MOUNT_PATH)
    {
        // initialize a default block size
        set_block_size(THINGER_OTA_CHUNK_SIZE);
    }

    virtual ~ThingerMbedOTA(){

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
        static FlashIAPBlockDevice bd_(XIP_BASE + 0xF00000, 0x100000);
        int err = fs_.mount(&bd_);
        if(err!=0){
            err = fs_.reformat(&bd_);
            if(err!=0){
                state["error"] = "cannot mount partition";
            }
        }
        if(err==0){
            reset_ota();
            f = fopen(FULL_UPDATE_FILE_PATH, "w+");
            if(f==nullptr){
                state["error"] = "cannot create file: " FULL_UPDATE_FILE_PATH;
            }
            return f!=nullptr;
        }
        return false;
    }

    bool reset_ota() override{
        // close current file (if any)
        if(f){
            fclose(f);
            f = nullptr;
        }
        // ensure no pending OTA files
        if(fs_.remove(FULL_UPDATE_FILE_PATH)==0){
            THINGER_DEBUG_VALUE("OTA", "Removed: ", FULL_UPDATE_FILE_PATH);
        }
        return true;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        return fwrite(buffer, 1, bytes, f) == bytes;
    }

    bool end_ota(pson& state) override{
        int err = fclose(f);
        if(err<0){
            state["error"] = "error while closing file";
            return false;
        }
        /*
        err = fs_.umount();
        if(err<0){
            state["error"] = "error on umount";
            return false;
        }
        */
        return true;
    }

private:
    mbed::FATFileSystem fs_;
    FILE *f = nullptr;

};

#endif