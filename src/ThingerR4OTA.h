#ifndef THINGER_R4_OTA_H
#define THINGER_R4_OTA_H

// OTA for the Arduino UNO R4 WiFi. The firmware is stored in the file system of the ESP32-S3
// module, which programs the RA4M1 with it when the device reboots after the update.
// Requires the UNO R4 WiFi firmware 0.4.1 or later (Tools > Firmware Updater).

#ifndef THINGER_OTA_CHUNK_SIZE
#define THINGER_OTA_CHUNK_SIZE 1024
#endif

#include "ThingerOTA.h"
#include "ThingerR4WiFi.h"
#include <WiFiFileSystem.h>

class ThingerR4OTA : public ThingerOTA{

    constexpr static const char * UPDATE_FILE_NAME = "/thinger_update.bin";

    // the ESP32-S3 module receives at most 255 bytes on every file system write
    static const size_t MAX_FILE_WRITE = 255;

public:
    ThingerR4OTA(ThingerR4WiFi& client) :
            ThingerOTA(client),
            client_(client)
    {
        // initialize a default block size
        set_block_size(THINGER_OTA_CHUNK_SIZE);
    }

    virtual ~ThingerR4OTA(){

    }

protected:

    void fill_options(pson& options) override{
        // set renesas platform
        options["platform"] = "renesas-ra";
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
        const char* fw_version = WiFi.firmwareVersion();
        THINGER_DEBUG_VALUE("OTA", "WiFi module firmware: ", fw_version);
        if(!firmware_supported(fw_version)){
            state["error"] = "Update the UNO R4 WiFi firmware to 0.4.1 or later";
            return false;
        }

        // cancel any update pending from a previous OTA
        client_.set_pending_update(nullptr);

        // mount the file system of the WiFi module, formatting it if required
        filesystem_.mount(true);
        return true;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
        // the first write creates (or truncates) the file, the following ones append to it
        size_t offset = 0;
        while(offset<bytes){
            size_t chunk = bytes - offset > MAX_FILE_WRITE ? MAX_FILE_WRITE : bytes - offset;
            int operation = firmware_offset_ + offset == 0 ? WIFI_FILE_WRITE : WIFI_FILE_APPEND;
            if(filesystem_.writefile(UPDATE_FILE_NAME, (const char*) buffer + offset, chunk, operation) != chunk){
                state["error"] = "Cannot write to the WiFi module file system";
                return false;
            }
            offset += chunk;
        }
        return true;
    }

    bool end_ota(pson& state) override{
        // the ESP32-S3 programs the RA4M1 when the device reboots
        client_.set_pending_update(UPDATE_FILE_NAME);
        return true;
    }

    bool reset_ota() override{
        client_.set_pending_update(nullptr);
        return true;
    }

private:

    // checks that the WiFi module firmware version is 0.4.1 or later
    static bool firmware_supported(const char* version){
        int major = 0, minor = 0, patch = 0;
        if(version==nullptr || sscanf(version, "%d.%d.%d", &major, &minor, &patch)<2) return false;
        return major>0 || minor>4 || (minor==4 && patch>=1);
    }

    ThingerR4WiFi& client_;
    WiFiFileSystem filesystem_;
};

#endif
