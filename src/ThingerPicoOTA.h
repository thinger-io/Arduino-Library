#ifndef THINGER_PICO_OTA_H
#define THINGER_PICO_OTA_H

// OTA for the Arduino-Pico core (Raspberry Pi Pico W, Pico 2 W). The firmware is stored in a
// LittleFS file and applied by the core OTA bootloader on reboot, so the sketch must be built
// with a filesystem large enough for the firmware (Tools > Flash Size, i.e. "Sketch: 1MB, FS: 1MB").
// The bootloader decompresses gzip images by itself: define THINGER_OTA_COMPRESSION 1 to request
// gzip images (requires a VSCode extension that labels gzip images as "gzip").
#ifndef THINGER_OTA_COMPRESSION
#define THINGER_OTA_COMPRESSION 0
#endif

#include "ThingerOTA.h"
#include <Updater.h>
#include "util/update_error.h"

class ThingerPicoOTA : public ThingerOTA{

public:
    ThingerPicoOTA(ThingerClient& client, size_t block_size=8192) :
        ThingerOTA(client)
    {
        set_block_size(block_size);
    }

    virtual ~ThingerPicoOTA(){

    }

protected:

    void fill_options(pson& options) override{
        // set raspberry pi platform
        options["platform"] = "raspberrypi";
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
#if THINGER_OTA_COMPRESSION
        // the file system keeps the image as received (compressed or not)
        if(is_compressed()) size = options["compressed_size"];
#endif

        bool init = Update.begin(size);

        // cannot init? try to end previous updates
        if(!init){
            THINGER_DEBUG("OTA", "Cannot Init... Clearing previous upgrade?");
            Update.end(true);
            init = Update.begin(size);
        }

        if(!init) fill_error(state);
        return init;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        if(Update.write(buffer, bytes) != bytes){
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

#if THINGER_OTA_COMPRESSION
    const char* supported_compression() override{
        return "gzip";
    }
#endif

private:

    void fill_error(pson& state){
        UpdateErrorGetter error;
        Update.printError(error);
        state["error"] = error.get_error();
    }

};

#endif
