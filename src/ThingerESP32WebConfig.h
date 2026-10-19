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

#ifndef THINGER_ESP32_WEBCONFIG_H
#define THINGER_ESP32_WEBCONFIG_H

#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include "ThingerESP32.h"
#include "ThingerWebConfig.h"

// the configuration is stored in NVS, that keeps it on power loss and does not depend on the partition scheme
#define THINGER_NVS_NAMESPACE "thinger"
#define THINGER_NVS_CONFIG "config"

class pson_spiffs_decoder : public protoson::pson_decoder{
public:
    pson_spiffs_decoder(File& file) : file_(file)
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

class pson_buffer_encoder : public protoson::pson_encoder{
public:
    pson_buffer_encoder(uint8_t* buffer, size_t size) : buffer_(buffer), size_(size)
    {
    }

protected:
    virtual bool write(const void* buffer, size_t size){
        if(written_+size > size_) return false;
        memcpy(buffer_ + written_, buffer, size);
        return protoson::pson_encoder::write(buffer, size);
    }

private:
    uint8_t* buffer_;
    size_t size_;
};

class pson_buffer_decoder : public protoson::pson_decoder{
public:
    pson_buffer_decoder(const uint8_t* buffer, size_t size) : buffer_(buffer), size_(size)
    {
    }

protected:
    virtual bool read(void* buffer, size_t size){
        if(read_+size > size_) return false;
        memcpy(buffer, buffer_ + read_, size);
        return protoson::pson_decoder::read(buffer, size);
    }

private:
    const uint8_t* buffer_;
    size_t size_;
};

class ThingerESP32WebConfig : public ThingerWebConfig<ThingerESP32>{

    public:
    ThingerESP32WebConfig(const char* user="", const char* device="", const char* credential="") : 
    ThingerWebConfig<ThingerESP32>(user, device, credential) {

    }

    bool clean_credentials() override{
        THINGER_DEBUG("_CONFIG", "Cleaning credentials...");
        bool removed = false;
        Preferences preferences;
        if(preferences.begin(THINGER_NVS_NAMESPACE)){
            if(preferences.isKey(THINGER_NVS_CONFIG)) removed = preferences.remove(THINGER_NVS_CONFIG);
            preferences.end();
        }
        // also remove the configuration of previous versions, so it is not migrated again
        removed |= remove_spiffs_configuration();
        THINGER_DEBUG("_CONFIG", removed ? "Config removed!" : "No config to remove!");
        return removed;
    }


    protected:

    bool save_configuration(pson& config) override{
        THINGER_DEBUG("_CONFIG", "Updating Device Info...");
        // get the encoded size first
        protoson::pson_encoder sizer;
        sizer.encode(config);
        size_t size = sizer.bytes_written();
        uint8_t* buffer = (uint8_t*) malloc(size);
        if(buffer==nullptr) return false;
        pson_buffer_encoder encoder(buffer, size);
        encoder.encode(config);
        bool result = false;
        Preferences preferences;
        if(encoder.bytes_written()==size && preferences.begin(THINGER_NVS_NAMESPACE)){
            // NVS replaces the value only when the new one is completely written
            result = preferences.putBytes(THINGER_NVS_CONFIG, buffer, size) == size;
            preferences.end();
        }
        free(buffer);
        THINGER_DEBUG("_CONFIG", result ? "Done!" : "Failed to save config!");
        return result;
    }

    bool load_configuration(pson& config) override{
        THINGER_DEBUG("_CONFIG", "Loading config from NVS...");
        Preferences preferences;
        if(preferences.begin(THINGER_NVS_NAMESPACE)){
            size_t size = preferences.isKey(THINGER_NVS_CONFIG) ? preferences.getBytesLength(THINGER_NVS_CONFIG) : 0;
            if(size>0){
                uint8_t* buffer = (uint8_t*) malloc(size);
                bool result = buffer!=nullptr && preferences.getBytes(THINGER_NVS_CONFIG, buffer, size) == size;
                preferences.end();
                if(result){
                    pson_buffer_decoder decoder(buffer, size);
                    result = decoder.decode(config);
                }
                free(buffer);
                return result;
            }
            preferences.end();
        }
        // migrate the configuration of previous versions, stored in SPIFFS
        if(load_spiffs_configuration(config)){
            THINGER_DEBUG("_CONFIG", "Migrating config from SPIFFS to NVS...");
            if(save_configuration(config)) remove_spiffs_configuration();
            return true;
        }
        return false;
    }

    private:

    // the file system is not formatted if it cannot be mounted, as it may hold other data
    bool load_spiffs_configuration(pson& config){
        if(!SPIFFS.begin(false)) return false;
        bool result = false;
        if(SPIFFS.exists(CONFIG_FILE)){
            File configFile = SPIFFS.open(CONFIG_FILE, "r");
            if(configFile){
                pson_spiffs_decoder decoder(configFile);
                result = decoder.decode(config);
                configFile.close();
            }
        }
        SPIFFS.end();
        return result;
    }

    bool remove_spiffs_configuration(){
        if(!SPIFFS.begin(false)) return false;
        bool removed = SPIFFS.exists(CONFIG_FILE) && SPIFFS.remove(CONFIG_FILE);
        SPIFFS.end();
        return removed;
    }

};



#endif
