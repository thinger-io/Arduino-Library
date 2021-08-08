#ifndef THINGER_ESP32OTA_H
#define THINGER_ESP32OTA_H

#include "ThingerOTA.h"
#include <Update.h>
#include "rom/miniz.h"

class ThingerESP32OTA : public ThingerOTA{

public:
    ThingerESP32OTA(ThingerClient& client) : 
        ThingerOTA(client)
    {

    }

    virtual ~ThingerESP32OTA(){

    }
    
private:
    bool enable_compression_ = true;
    size_t remaining_compressed_ = 0;
    tinfl_decompressor inflator_;

public:

    void set_compression(bool compression){
        enable_compression_ = compression;
    }

protected:

    void fill_options(pson& options) override{
        options["platform"] = "espressif32";
        if(enable_compression_){
            options["compression"] = "gzip";
        }
    }

    bool begin_ota(
        const char* firmware, 
        const char* version, 
        size_t bytes_size,
        size_t compressed_size) override
    {

        bool init = Update.begin(bytes_size);

        // try to end previous updates
        if(!init){
            THINGER_DEBUG("OTA", "Cannot Init... Clearing previous upgrade?");
            Update.abort();
            init = Update.begin(bytes_size);
        }

        // initialize compression parameters (0 disabled)
        remaining_compressed_ = compressed_size;
        if(init && remaining_compressed_){
            inflator_ = tinfl_decompressor{};
        }

        return init;
    }

    bool write_ota(const char* buffer, size_t bytes) override{
        return remaining_compressed_ ?
            write_ota_compressed(buffer, bytes) : 
            Update.write((uint8_t*) buffer, bytes) == bytes;
    }

    bool write_ota_compressed(const char* buffer, size_t bytes){
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

            status = tinfl_decompress(&inflator_, (uint8_t*) buffer, &in_bytes,
                            out_buf, next_out, &out_bytes,
                            flags);

            remaining_compressed_ -= in_bytes;
            bytes -= in_bytes;
            buffer += in_bytes;

            next_out += out_bytes;
            size_t bytes_in_out_buf = next_out - out_buf;
            if (status <= TINFL_STATUS_DONE || bytes_in_out_buf == sizeof(out_buf)) {
                size_t write = Update.write((uint8_t*) out_buf, bytes_in_out_buf);
                if(write!=bytes_in_out_buf) return false;
                next_out = out_buf;
            }
        } // while
        
        // success if all bytes where consumed
        return bytes==0;
    }

    size_t remaining() override{
        return Update.remaining();
    }

    bool end_ota() override{
        return Update.end();
    }

};

#endif