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
        f = nullptr;
        if(err<0){
            state["error"] = "error while closing file";
            return false;
        }
        // the second stage bootloader (SFU) ignores a firmware without it, so the device would reboot with the
        // current firmware: the new firmware must include ThingerMbedOTA.h (that links SFU) to allow updates
        if(!has_sfu()){
            reset_ota();
            THINGER_DEBUG("OTA", "The firmware does not include ThingerMbedOTA.h (SFU)");
            state["error"] = "The firmware does not include ThingerMbedOTA.h, so it cannot be applied";
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

    // checks the SFU signature in the first 64KB of the update file, as the SFU bootloader does
    bool has_sfu(){
        static const char signature[] = "SFU version";
        const size_t signature_size = sizeof(signature) - 1;
        FILE* file = fopen(FULL_UPDATE_FILE_PATH, "rb");
        if(file==nullptr) return false;
        char buffer[512 + sizeof(signature)];
        size_t kept = 0, total = 0;
        bool found = false;
        while(!found && total < 0x10000){
            size_t read = fread(buffer + kept, 1, 512, file);
            if(read==0) break;
            total += read;
            size_t size = kept + read;
            for(size_t i=0; !found && i + signature_size <= size; i++){
                found = memcmp(buffer + i, signature, signature_size) == 0;
            }
            // keep the last bytes, in case the signature is split between two reads
            kept = size < signature_size - 1 ? size : signature_size - 1;
            memmove(buffer, buffer + size - kept, kept);
        }
        fclose(file);
        return found;
    }

    mbed::FATFileSystem fs_;
    FILE *f = nullptr;

};

#endif