#ifndef THINGER_ESP32OTA_H
#define THINGER_ESP32OTA_H

#ifndef THINGER_OTA_COMPRESSION
#define THINGER_OTA_COMPRESSION 1
#endif

#include "ThingerOTA.h"
#include <Update.h>

#if THINGER_OTA_COMPRESSION
// ESP-IDF 5.5 (Arduino ESP32 core 3.3) exposes miniz.h directly from esp_rom/include
#if __has_include("rom/miniz.h")
#include "rom/miniz.h"
#elif __has_include("miniz.h")
#include "miniz.h"
#else
#error "miniz.h not found: define THINGER_OTA_COMPRESSION 0 to disable OTA compression"
#endif
#endif

#ifdef THINGER_OTA_ROLLBACK
#include <esp_ota_ops.h>

// seconds that a new firmware has to connect to Thinger.io before rolling back to the previous one
#ifndef THINGER_OTA_ROLLBACK_TIMEOUT
    #define THINGER_OTA_ROLLBACK_TIMEOUT 300
#endif

// the Arduino core validates a new firmware on boot unless this hook returns true: ThingerESP32OTA validates it
// once it connects to Thinger.io (define THINGER_OTA_ROLLBACK in a single source file)
extern "C" bool verifyRollbackLater(){
    return true;
}
#endif

class ThingerESP32OTA : public ThingerOTA{

public:
    ThingerESP32OTA(ThingerClient& client, size_t block_size=32768) : 
        ThingerOTA(client)
    {
        // initialize a default block size for esp32
        set_block_size(block_size);
#ifdef THINGER_OTA_ROLLBACK
        start_rollback_check(client);
#endif
    }

    virtual ~ThingerESP32OTA(){

    }

#ifdef THINGER_OTA_ROLLBACK
    // a firmware installed by OTA is pending verification until it connects to Thinger.io: otherwise, the
    // bootloader rolls back to the previous firmware after the timeout, or if the device reboots before
    static void start_rollback_check(ThingerClient& client){
        esp_ota_img_states_t state;
        if(esp_ota_get_state_partition(esp_ota_get_running_partition(), &state)!=ESP_OK) return;
        if(state!=ESP_OTA_IMG_PENDING_VERIFY) return;
        xTaskCreate([](void* param){
            ThingerClient* client = (ThingerClient*) param;
            unsigned long start = millis();
            while(client->get_state()!=ThingerClient::THINGER_AUTHENTICATED){
                if(millis()-start >= THINGER_OTA_ROLLBACK_TIMEOUT*1000UL){
                    THINGER_DEBUG("OTA", "The new firmware cannot connect to Thinger.io, rolling back to the previous one...");
                    esp_ota_mark_app_invalid_rollback_and_reboot();
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            THINGER_DEBUG("OTA", "The new firmware connected to Thinger.io, so it is valid");
            esp_ota_mark_app_valid_cancel_rollback();
            vTaskDelete(nullptr);
        }, "thinger_rollback", 4096, &client, 1, nullptr);
    }
#endif

    void fill_options(pson& options) override{
        // set espressif32 platform
        options["platform"] = "espressif32";

        // request a md5 checksum
        options["checksum"] = "md5";
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

#if THINGER_OTA_COMPRESSION
        if(is_compressed()){
            // initialize compression parameters (0 disabled)
            remaining_compressed_ = options["compressed_size"];
            if(init && remaining_compressed_){
                inflator_ = tinfl_decompressor{};
            }
        }
#endif

        return init;
    }

#if THINGER_OTA_COMPRESSION
    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        if(!is_compressed()){
            // ensure the compression is enabled
            return do_write(buffer, bytes, state);
        }

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
                update_firmware_checksum(out_buf, bytes_in_out_buf);
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

#if THINGER_OTA_COMPRESSION
    const char* supported_compression() override{
        return "zlib";
    }
#endif

    bool do_write(uint8_t* buffer, size_t bytes, pson& state){
        if(Update.write(buffer, bytes) != bytes){
            state["error"] = Update.errorString();
            return false;
        }
        return true;
    }

private:
    char md5_checksum_[33];
#if THINGER_OTA_COMPRESSION
    size_t remaining_compressed_ = 0;
    tinfl_decompressor inflator_;
#endif

};

#endif