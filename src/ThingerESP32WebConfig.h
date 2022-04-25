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
#define FORMAT_SPIFFS_IF_FAILED true

#include <FS.h>
#include <SPIFFS.h>
#include "ThingerESP32.h"
#include "ThingerWebConfig.h"


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

class pson_spiffs_encoder : public protoson::pson_encoder{
public:
    pson_spiffs_encoder(File& file) : file_(file)
    {
    }

protected:
    virtual bool write(const void* buffer, size_t size){
        size_t wrote = file_.write((const uint8_t*)buffer, size);
        protoson::pson_encoder::write(buffer, wrote);
        return wrote == size;
    }

private:
    File& file_;
};

class ThingerESP32WebConfig : public ThingerWebConfig<ThingerESP32>{

    public:
    ThingerESP32WebConfig(const char* user="", const char* device="", const char* credential="") : 
    ThingerWebConfig<ThingerESP32>(user, device, credential) {

    }

     bool clean_credentials() override{
        THINGER_DEBUG("_CONFIG", "Cleaning credentials...");
        // clean Thinger.io credentials from file system
        if(SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
            THINGER_DEBUG("_CONFIG", "FS Mounted!");
            if (SPIFFS.exists(CONFIG_FILE)) {
                THINGER_DEBUG("_CONFIG", "Removing Config File...");
                if(SPIFFS.remove(CONFIG_FILE)){
                    THINGER_DEBUG("_CONFIG", "Config file removed!");
                    return true;
                }else{
                    THINGER_DEBUG("_CONFIG", "Cannot delete config file!");
                }
            }else{
                THINGER_DEBUG("_CONFIG", "No config file to delete!");
            }
            SPIFFS.end();
        }
        return false;
    }


    protected:

    bool save_configuration(pson& config) override{
        THINGER_DEBUG("_CONFIG", "Updating Device Info...");
        if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
            File configFile = SPIFFS.open(CONFIG_FILE, "w");
            if (configFile) {
                pson_spiffs_encoder encoder(configFile);
                encoder.encode(config);
                configFile.close();
                THINGER_DEBUG("_CONFIG", "Done!");
                return true;
            } else {
                THINGER_DEBUG("_CONFIG", "Failed to open config file for writing!");
            }
            SPIFFS.end();
        }
        return false;
    }

    bool load_configuration(pson& config) override{
        THINGER_DEBUG("_CONFIG", "Mounting FS...");
        if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
            THINGER_DEBUG("_CONFIG", "FS Mounted!");
            if (SPIFFS.exists(CONFIG_FILE)) {
                //file exists, reading and loading
                THINGER_DEBUG("_CONFIG", "Opening Config File...");
                File configFile = SPIFFS.open(CONFIG_FILE, "r");
                if(configFile){
                    THINGER_DEBUG("_CONFIG", "Config File is Open!");
                    pson_spiffs_decoder decoder(configFile);
                    bool result = decoder.decode(config);
                    configFile.close();
                    return result;
                }else{
                    THINGER_DEBUG("_CONFIG", "Config File is Not Available!");
                }
            }
            // close SPIFFS
            SPIFFS.end();
        } else {
            THINGER_DEBUG("_CONFIG", "Failed to Mount FS!");
        }
        return false;
    }

};



#endif