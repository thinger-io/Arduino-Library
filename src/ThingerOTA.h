#ifndef THINGER_OTA_H
#define THINGER_OTA_H

#include "ThingerClient.h"

#if THINGER_OTA_MD5_VERIFICATION
#include "util/md5.h"
#endif

class ThingerOTA{

public:

    ThingerOTA(ThingerClient& client){

        // resource for configuring ota options
        client["$ota"]["options"] >> [&](pson& options){
            options["enabled"] = is_enabled();
            options["block_size"] = get_block_size();
            options["timeout"] = timeout_;
#if THINGER_OTA_MD5_VERIFICATION
            options["checksum"] = "md5";
#endif 
            fill_options(options);
        };

        // resource for starting ota updates
        (client["$ota"]["begin"] = [&](pson& in, pson& out){
            if(in.is_empty() || running_){
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
                // initialize ota control variables 
                running_ = true;
                succeed_ = false;
            }

        }).then([&](){
            if(running_){

                if(on_start_) on_start_();
                
                // stop streams in case any is running
                THINGER_DEBUG("OTA", "Stopping streams...");
                client.stop_streams();

                // initialize ota control variables 
                running_ = true;
                auto last_remaining = remaining();
                auto last_update = millis();
        
                THINGER_DEBUG("OTA", "Waiting for firmware...");

                // while ota is running
                while(running_){

                    // call client handle to allow incoming requests
                    client.handle();
                    
                    // if there is progress while running ota... everything is ok
                    auto current_remaining = remaining();
                    if(last_remaining!=current_remaining){
                        last_remaining = current_remaining;
                        last_update = millis();

                    // if there is no progress and elapsed time is greater than timeout -> stop
                    }else if(millis()-last_update>=timeout_){
                        THINGER_DEBUG_VALUE("OTA", "OTA update timed out after (ms): ", millis()-last_update);
                        running_ = false;
                        break;
                    }

                    yield();
                }

                if(!succeed_){
                    THINGER_DEBUG("OTA", "OTA update failed... Restarting device");
                    client.reboot();
                }            
            }
        });

        // resource for writing partial ota data
        (client["$ota"]["write"] = [&](pson& in, pson& out){
            // ensure we are running the ota upgrade
            if(!running_){
                THINGER_DEBUG("OTA", "no OTA update in progress");
                out["success"] = false;
                out["error"] = "no OTA update in progress";
                return;
            }

            // ensure incoming data is a binary payload
            if(!in.is_bytes()){
                THINGER_DEBUG("OTA", "Invalid input type received");
                out["success"] = false;
                out["error"] = "Invalid input type received";
                running_ = false;
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

            running_ = success;
            out["success"] = success;
        }).then([&]{
            if(running_){
                if(on_progress_) on_progress_(firmware_offset_, firmware_size_);
            }
        });

        // resource for ending ota update
        (client["$ota"]["end"] >> [&](pson& out){
            
            // ensure we are running the ota upgrade
            if(!running_){
                THINGER_DEBUG("OTA", "No OTA update in progress");
                out["success"] = false;
                out["error"] = "No OTA update in progress";
                return;
            }

            // ensure we have received everything
            if(remaining()){
                THINGER_DEBUG("OTA", "Cannot end OTA with missing bytes");
                out["success"] = false;
                out["error"] = "Cannot end OTA with missing bytes";
                return;
            }

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
            succeed_ = result;
            THINGER_DEBUG_VALUE("OTA", "Update OK: ", result);
            out["success"] = result;
        }).then([&]{
            if(running_){
                if(on_end_) on_end_();
            }
        });

        // resource for rebooting the device after ota update
        (client["$ota"]["reboot"] = [](){
            THINGER_DEBUG("OTA", "Received reboot request...");
            // do nothing here and delay execution after ok response is sent
        }).then([&](){
            THINGER_DEBUG("OTA", "Rebooting...");
            client.reboot();
        });

    }

    ~ThingerOTA(){

    }

    void set_block_size(size_t size){
        THINGER_DEBUG_VALUE("OTA", "Setting OTA block size: ", size);
        block_size_ = size;
    }


    void set_timeout(size_t timeout){
        timeout_ = timeout;
    }

    bool is_enabled(){
        return enabled_;
    }

    void set_enabled(bool enabled){
        enabled_ = enabled;
    }

    void on_start(std::function<void()> on_start){
        on_start_ = on_start;
    }

    void on_end(std::function<void()> on_end){
        on_end_ = on_end;
    }

    void on_progress(std::function<void(size_t progress, size_t total)> on_progress){
        on_progress_ = on_progress;
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

    virtual size_t get_block_size() const{
        return block_size_;
    }

    virtual void fill_options(pson& options) = 0;

    bool enabled_           = true;
    bool running_           = false;
    bool succeed_           = false;

    uint16_t block_size_    = 8192;
    size_t timeout_         = 30000;
    size_t firmware_offset_ = 0;
    size_t firmware_size_   = 0;

    std::function<void()> on_start_;
    std::function<void(size_t progress, size_t total)> on_progress_;
    std::function<void()> on_end_;

#if THINGER_OTA_MD5_VERIFICATION
    MD5_CTX md5_ctx_;
    String md5_checksum_;
#endif

};

#endif