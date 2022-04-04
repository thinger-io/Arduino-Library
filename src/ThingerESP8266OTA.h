#ifndef THINGER_ESP8266OTA_H
#define THINGER_ESP8266OTA_H

#include "ThingerOTA.h"
#include <Updater.h>

// helper class to get message from update error 
class UpdateErrorGetter : public Print{
public:
    UpdateErrorGetter() {}
    virtual ~UpdateErrorGetter(){}

    virtual size_t write(uint8_t value){
        if(index_<BUFFER_SIZE-1){
            buffer_[index_++] = value;
            return 1;
        }else{
            return 0;
        }
    }

    const char* get_error(){
        buffer_[index_]=0;
        return buffer_;
    }

private:
    static const size_t BUFFER_SIZE = 128;
    char buffer_[BUFFER_SIZE];
    size_t index_ = 0;
};

class ThingerESP8266OTA : public ThingerOTA{

public:
    ThingerESP8266OTA(ThingerClient& client) : 
        ThingerOTA(client)
    {
        // initialize a default block size for ESP8266
        set_block_size(2048);
    }

    virtual ~ThingerESP8266OTA(){

    }
    
private:
    char md5_checksum_[33];

protected:

    /*
    size_t get_block_size() const override{
        size_t free_heap = ESP.getFreeHeap() - 512;
        THINGER_DEBUG_VALUE("OTA", "Initializing default block size to: %zu", free_heap);
        return 2048;
    }*/

    void fill_options(pson& options) override{
        // set esp8266 platform
        options["platform"] = "espressif8266";

        // request a md5 checksum
        options["checksum"] = "md5";

        // add gzip compression option if ATOMIC_FS_UPDATE is defined while compiling
#ifdef ATOMIC_FS_UPDATE
        options["compression"] = "gzip";
#endif
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {

#ifdef ATOMIC_FS_UPDATE
        size = options["compressed_size"];
#endif 

        bool init = Update.begin(size);

        // cannot init? try to end previous updates
        if(!init){
            THINGER_DEBUG("OTA", "Cannot Init... Clearing previous upgrade?");
            Update.end(true);
            init = Update.begin(size);
        }

        if(!init){
            fill_error(state);
            return false;
        }

        // try the checksum depending on compression 
#ifdef ATOMIC_FS_UPDATE
        const char* checksum = options["compressed_checksum"];
#else
        const char* checksum = options["checksum"];
#endif
        if(strlen(checksum)==32){
            THINGER_DEBUG_VALUE("OTA", "Set MD5 verification: ", checksum);
            strcpy(md5_checksum_, checksum);
            Update.setMD5(md5_checksum_);
        }

        return init;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        if(Update.write((uint8_t*) buffer, bytes) != bytes){
            fill_error(state);
            return false;
        }
        return true;
    }

    size_t remaining() override{
        return Update.remaining();
    }

    bool end_ota(pson& state) override{
        if(!Update.end()){
            fill_error(state);
            return false;
        }
        return true;
    }

private:

    void fill_error(pson& state){
        UpdateErrorGetter error;
        Update.printError(error);
        state["error"] = error.get_error();
    }

};

#endif