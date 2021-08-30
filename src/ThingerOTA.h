#ifndef THINGER_OTA_H
#define THINGER_OTA_H

#include "ThingerClient.h"

#if THINGER_OTA_MD5_VERIFICATION
#include "util/md5.h"
#endif

class ThingerOTA{

public:

    ThingerOTA(ThingerClient& client){

        client["$ota"]["options"] >> [&](pson& options){
            options["enabled"] = enabled_;
            options["block_size"] = block_size_;
#if THINGER_OTA_MD5_VERIFICATION
            options["checksum"] = "md5";
#endif 
            fill_options(options);
        };

        client["$ota"]["begin"] = [&](pson& in, pson& out){
            if(in.is_empty()){
                out["success"] = false;
                return;
            }

            const char* firmware   = in["firmware"];
            const char* version    = in["version"];
            firmware_size_         = in["size"];
            size_t chunk_size      = in["chunk_size"];
            firmware_offset_ = 0;

            THINGER_DEBUG("OTA", "Received OTA request...");
            THINGER_DEBUG_VALUE("OTA", "Firmware: ", firmware);
            THINGER_DEBUG_VALUE("OTA", "Firmware Size: ", firmware_size_);
            THINGER_DEBUG_VALUE("OTA", "Chunk Size: ", chunk_size);

#if THINGER_OTA_MD5_VERIFICATION
            // use checksum if provided in the begin options. ESP32 uses original checksum 
            // for the verification, as it will write firmware after deflated in memory
            const char* checksum = in["checksum"];
            if(strlen(checksum)==32){
                THINGER_DEBUG_VALUE("OTA", "MD5 checksum: ", checksum);
                md5_checksum_ = checksum;
                MD5::MD5Init(&md5_ctx_);
            }
#endif

            THINGER_DEBUG("OTA", "Initializing update...");

            bool init = begin_ota(firmware, version, firmware_size_, in, out);

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
            auto success = write_ota((uint8_t*)buffer, size, out);
            auto stop = millis();

            if(success){
                firmware_offset_ += size;
#if THINGER_OTA_MD5_VERIFICATION
                MD5::MD5Update(&md5_ctx_, (uint8_t*)buffer, size);
#endif
            }

            THINGER_DEBUG_VALUE("OTA", "Wrote OK: ", success);
            THINGER_DEBUG_VALUE("OTA", "Elapsed time ms: ", stop-start);

            out["success"] = success;
        };

        client["$ota"]["end"] >> [&](pson& out){
            THINGER_DEBUG("OTA", "Finishing...");

            bool result = true;

#if THINGER_OTA_MD5_VERIFICATION
            unsigned char hash[16] = {0};
            MD5::MD5Final(hash, &md5_ctx_);
            String md5_hash = "";
            md5_hash.reserve(32);
            for(auto i=0; i<16; i++){
                if(hash[i]<=15) md5_hash += '0';
                md5_hash += String(hash[i], HEX);
            }
            THINGER_DEBUG_VALUE("OTA", "Computed MD5: ", md5_hash);
            if(!md5_hash.equalsIgnoreCase(md5_checksum_)){
                out["error"] = "Invalid MD5 checksum";
                THINGER_DEBUG("OTA", "Invalid MD5 checksum");
                reset_ota();
                result = false;
            }
            THINGER_DEBUG("OTA", "MD5 verification succeed!");
#endif

            result = result && end_ota(out);

            THINGER_DEBUG_VALUE("OTA", "Update OK: ", result);
            out["success"] = result;
        };

        client["$ota"]["reboot"] = [&](){
            client.reboot();
        };
    
    }

    ~ThingerOTA(){

    }

    void set_block_size(size_t size){
        block_size_ = size;
    }

    void set_enabled(bool enabled){
        enabled_ = enabled;
    }


protected:

    // methods to be implemented by underlying device
    virtual bool begin_ota(const char* firmware, const char* version, size_t bytes_size, pson& options, pson& state) = 0;
    virtual bool write_ota(uint8_t* buffer, size_t bytes, pson& state) = 0;
    virtual bool reset_ota(){
        return true;
    }

    virtual size_t remaining(){
        return firmware_size_ - firmware_offset_;
    }

    virtual bool end_ota(pson& state){
        return true;
    }

    virtual void fill_options(pson& options) = 0;


    bool enabled_           = true;
    uint16_t block_size_    = 8192;
    size_t firmware_offset_ = 0;
    size_t firmware_size_   = 0;


#if THINGER_OTA_MD5_VERIFICATION
    MD5_CTX md5_ctx_;
    String md5_checksum_;
#endif

};

#endif