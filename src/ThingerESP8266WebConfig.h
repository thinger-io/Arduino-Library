// The MIT License (MIT)
//
// Copyright (c) 2017 THINK BIG LABS SL
// Author: alvarolb@gmail.com (Alvaro Luis Bustamante)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef THINGER_ESP8266_WEBCONFIG_H
#define THINGER_ESP8266_WEBCONFIG_H

#include <LittleFS.h>
#include <flash_hal.h>
#include "ThingerESP8266.h"
#include "ThingerWebConfig.h"

class pson_littlefs_decoder : public protoson::pson_decoder{
public:
    pson_littlefs_decoder(File& file) : file_(file)
    {
    }

protected:
    virtual bool read(void* buffer, size_t size){
        size_t read = file_.readBytes((char*)buffer, size);
        protoson::pson_decoder::read(buffer, read);
        return read == size;
    }

private:
    File& file_;
};

class pson_littlefs_encoder : public protoson::pson_encoder{
public:
    pson_littlefs_encoder(File& file) : file_(file)
    {
    }

protected:
    virtual bool write(const void* buffer, size_t size){
        size_t wrote = file_.write((const uint8_t*)buffer, size);
        protoson::pson_encoder::write(buffer, wrote);
        if(wrote != size) success_ = false;
        return wrote == size;
    }

public:
    bool success() const { return success_; }

private:
    File& file_;
    bool success_ = true;
};


class ThingerESP8266WebConfig : public ThingerWebConfig<ThingerESP8266>{

    public:

    ThingerESP8266WebConfig(const char* user="", const char* device="", const char* credential="") : 
    ThingerWebConfig<ThingerESP8266>(user, device, credential) {

    }

    bool clean_credentials() override{
        THINGER_DEBUG("_CONFIG", "Cleaning credentials...");
        FS* storage = mount_fs();
        if(!storage) return false;
        bool removed = storage->exists(CONFIG_FILE) && storage->remove(CONFIG_FILE);
        if(storage!=&LittleFS) storage->end();
        THINGER_DEBUG("_CONFIG", removed ? "Config file removed!" : "No config file to delete!");
        return removed;
    }


    protected:

    bool save_configuration(pson& config) override{
        THINGER_DEBUG("_CONFIG", "Updating Device Info...");
        FS* storage = mount_fs();
        if(!storage) return false;
        // Keep the previous configuration until the complete replacement is ready.
        const char* temporary = CONFIG_FILE ".tmp";
        File configFile = storage->open(temporary, "w");
        bool result = false;
        if (configFile) {
            pson_littlefs_encoder encoder(configFile);
            encoder.encode(config);
            configFile.flush();
            bool written = encoder.success();
            configFile.close();
            if(written) result = storage->rename(temporary, CONFIG_FILE);
        }
        if(!result) storage->remove(temporary);
        if(storage!=&LittleFS) storage->end();
        THINGER_DEBUG("_CONFIG", result ? "Done!" : "Failed to save config!");
        return result;
    }

    bool load_configuration(pson& config) override{
        THINGER_DEBUG("_CONFIG", "Mounting FS...");
        FS* storage = mount_fs();
        if(!storage){
            THINGER_DEBUG("_CONFIG", "Failed to Mount FS!");
            return false;
        }
        bool result = false;
        File configFile = storage->exists(CONFIG_FILE) ? storage->open(CONFIG_FILE, "r") : File();
        if(configFile){
            pson_littlefs_decoder decoder(configFile);
            result = decoder.decode(config);
            configFile.close();
        }
        if(storage!=&LittleFS) storage->end();
        return result;
    }

    private:

    // Borrow a sketch mount, otherwise mount the same partition using a private
    // non-formatting adapter. The global LittleFS configuration has no getter.
    static FS* mount_fs(){
        FSInfo info;
        if(LittleFS.info(info)) return &LittleFS;
        static FS storage(fs::FSImplPtr(new littlefs_impl::LittleFSImpl(
            FS_PHYS_ADDR, FS_PHYS_SIZE, FS_PHYS_PAGE, FS_PHYS_BLOCK, 1)));
        if(!storage.setConfig(LittleFSConfig(false)) || !storage.begin()) return nullptr;
        return &storage;
    }

};



#endif