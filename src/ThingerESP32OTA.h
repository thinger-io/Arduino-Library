#ifndef THINGER_ESP32OTA_H
#define THINGER_ESP32OTA_H

#include "ThingerOTA.h"
#include <Update.h>

#if THINGER_ESP32OTA_COMPRESSION
#include "rom/miniz.h"
#endif

class ThingerESP32OTA : public ThingerOTA{

public:
    ThingerESP32OTA(ThingerClient& client) : 
        ThingerOTA(client)
    {
        // initialize a default block size for esp32
        set_block_size(32768);
    }

    virtual ~ThingerESP32OTA(){

    }

    void fill_options(pson& options) override{
        // set espressif32 platform
        options["platform"] = "espressif32";

        // request a md5 checksum
        options["checksum"] = "md5";

        // add gzip compression option if THINGER_ESP32OTA_UNCOMPRESSED is not defined
#if THINGER_ESP32OTA_COMPRESSION
        options["compression"] = "zlib";
#endif
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
        // ESP32 init with original firmware size (deflated in memory and then issue regular updates)
        bool init = Update.begin(size);

        // try to end previous updates
        if(!init){
            THINGER_DEBUG("OTA", "Cannot Init... Clearing previous upgrade?");
            Update.abort();
            init = Update.begin(size);
        }

        if(!init){
            state["error"] = Update.errorString();
            return false;
        }

        // use checksum if provided in the begin options. ESP32 uses original checksum 
        // for the verification, as it will write firmware after deflated in memory
        const char* checksum = options["checksum"];
        if(strlen(checksum)==32){
            THINGER_DEBUG_VALUE("OTA", "Set MD5 verification:", checksum);
            strcpy(md5_checksum_, checksum);
            Update.setMD5(md5_checksum_);
        }

#if THINGER_ESP32OTA_COMPRESSION
        // initialize compression parameters (0 disabled)
        remaining_compressed_ = options["compressed_size"];
        if(init && remaining_compressed_){
            inflator_ = tinfl_decompressor{};
        }
#endif

        return init;
    }

#if THINGER_ESP32OTA_COMPRESSION
    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        static uint8_t out_buf[32768];
        static uint8_t *next_out = out_buf;
        int status = TINFL_STATUS_NEEDS_MORE_INPUT;

        while(bytes > 0 && Update.remaining() > 0 && status > TINFL_STATUS_DONE) {
            size_t in_bytes = bytes; /* input remaining */
            size_t out_bytes = out_buf + sizeof(out_buf) - next_out; /* output space remaining */
            int flags = TINFL_FLAG_PARSE_ZLIB_HEADER;
            if(remaining_compressed_> bytes) {
                flags |= TINFL_FLAG_HAS_MORE_INPUT;
            }

            status = tinfl_decompress(&inflator_, buffer, &in_bytes,
                            out_buf, next_out, &out_bytes,
                            flags);

            remaining_compressed_ -= in_bytes;
            bytes -= in_bytes;
            buffer += in_bytes;

            next_out += out_bytes;
            size_t bytes_in_out_buf = next_out - out_buf;
            if (status == TINFL_STATUS_DONE || bytes_in_out_buf == sizeof(out_buf)) {
                if(!do_write(out_buf, bytes_in_out_buf, state)) return false;
                next_out = out_buf;
            }
        } // while

        if (status < TINFL_STATUS_DONE) {
            state["error"] = "Error while decompressing!";
            return false;
        }
        
        // success if all bytes where consumed
        return bytes==0;
    }       
#else
    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        return do_write(buffer, bytes, state);
    } 
#endif


    size_t remaining() override{
        return Update.remaining();
    }

    bool end_ota(pson& state) override{
        if(!Update.end()){
            state["error"] = Update.errorString();
            return false;
        }
        return true;
    }

protected: 

    bool do_write(uint8_t* buffer, size_t bytes, pson& state){
        if(Update.write(buffer, bytes) != bytes){
            state["error"] = Update.errorString();
            return false;
        }
        return true;
    }

private:
    char md5_checksum_[33];
#if THINGER_ESP32OTA_COMPRESSION
    size_t remaining_compressed_ = 0;
    tinfl_decompressor inflator_;
#endif

};

#endif