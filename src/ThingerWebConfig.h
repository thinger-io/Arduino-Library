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

#ifndef THINGER_WEB_CONFIG_H
#define THINGER_WEB_CONFIG_H

#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include "ThingerClient.h"

#define CONFIG_FILE "/config.pson"

#ifndef THINGER_DEVICE_SSID
    #define THINGER_DEVICE_SSID "Thinger-Device"
#endif

#ifndef THINGER_DEVICE_SSID_PSWD
    #define THINGER_DEVICE_SSID_PSWD "thinger.io"
#endif

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

class ThingerWebConfig : public ThingerClient {

public:
    ThingerWebConfig() : ThingerClient(client_, user, device, device_credential)
    {
        // initialize empty configuration
        user[0] = '\0';
        device[0] = '\0';
        device_credential[0] = '\0';
    }

    ~ThingerWebConfig(){

    }



    void clean_credentials(){
        THINGER_DEBUG("_CONFIG", "Cleaning credentials...");
        // clean Thinger.io credentials from file system
        if(SPIFFS.begin()) {
            THINGER_DEBUG("_CONFIG", "FS Mounted!");
            if (SPIFFS.exists(CONFIG_FILE)) {
                THINGER_DEBUG("_CONFIG", "Removing Config File...");
                if(SPIFFS.remove(CONFIG_FILE)){
                    THINGER_DEBUG("_CONFIG", "Config file removed!");
                }else{
                    THINGER_DEBUG("_CONFIG", "Cannot delete config file!");
                }
            }else{
                THINGER_DEBUG("_CONFIG", "No config file to delete!");
            }
            SPIFFS.end();
        }
    }

protected:

    virtual bool network_connected(){
        return WiFi.status() == WL_CONNECTED && !(WiFi.localIP() == INADDR_NONE);
    }

    virtual bool connect_network(){
        bool thingerCredentials = false;

        // read current thinger.io credentials from file system
        THINGER_DEBUG("_CONFIG", "Mounting FS...");
        if (SPIFFS.begin()) {
            THINGER_DEBUG("_CONFIG", "FS Mounted!");
            if (SPIFFS.exists(CONFIG_FILE)) {
                //file exists, reading and loading
                THINGER_DEBUG("_CONFIG", "Opening Config File...");
                File configFile = SPIFFS.open("/config.pson", "r");
                if(configFile){
                    THINGER_DEBUG("_CONFIG", "Config File is Open!");
                    pson_spiffs_decoder decoder(configFile);
                    pson config;
                    decoder.decode(config);
                    configFile.close();

                    THINGER_DEBUG("_CONFIG", "Config File Decoded!");
                    strcpy(user, config["user"]);
                    strcpy(device, config["device"]);
                    strcpy(device_credential, config["credential"]);

                    thingerCredentials = true;

                    THINGER_DEBUG_VALUE("_CONFIG", "User: ", user);
                    THINGER_DEBUG_VALUE("_CONFIG", "Device: ", device);
                    THINGER_DEBUG_VALUE("_CONFIG", "Credential: ", device_credential);
                }else{
                    THINGER_DEBUG("_CONFIG", "Config File is Not Available!");
                }
            }
            // close SPIFFS
            SPIFFS.end();
        } else {
            THINGER_DEBUG("_CONFIG", "Failed to Mount FS!");
        }

        // if credentials and wifi already configured, try to connect to wifi
        if(thingerCredentials && WiFi.SSID() && !WiFi.SSID().length()==0){
            THINGER_DEBUG("_CONFIG", "Connecting using previous configuration...");
            unsigned long wifi_timeout = millis();
            THINGER_DEBUG_VALUE("NETWORK", "Connecting to network ", WiFi.SSID());
            WiFi.begin();
            while(WiFi.status() != WL_CONNECTED) {
                if(millis() - wifi_timeout > 30000) return false;
                yield();
            }
        // don't have credentials or Wifi, initiate config portal
        }else{
            THINGER_DEBUG("_CONFIG", "Starting Webconfig...");

            WiFiManager wifiManager;

#ifdef _DEBUG_
            wifiManager.setDebugOutput(true);
#else
            wifiManager.setDebugOutput(false);
#endif

            // define additional wifi parameters
            WiFiManagerParameter user_parameter("user", "User Id", user, 40);
            WiFiManagerParameter device_parameter("device", "Device Id", device, 40);
            WiFiManagerParameter credential_parameter("credential", "Device Credential", device_credential, 40);
            wifiManager.addParameter(&user_parameter);
            wifiManager.addParameter(&device_parameter);
            wifiManager.addParameter(&credential_parameter);

            bool wifiConnected = wifiManager.startConfigPortal(THINGER_DEVICE_SSID, THINGER_DEVICE_SSID_PSWD);

            if (!wifiConnected) {
                WiFi.disconnect();
                THINGER_DEBUG("NETWORK", "Configuration Failed! Resetting...");
                delay(3000);
                ESP.reset();
                return false;
            }

            //read updated parameters
            strcpy(user, user_parameter.getValue());
            strcpy(device, device_parameter.getValue());
            strcpy(device_credential, credential_parameter.getValue());

            THINGER_DEBUG("_CONFIG", "Updating Device Info...");
            if (SPIFFS.begin()) {
                File configFile = SPIFFS.open(CONFIG_FILE, "w");
                if (configFile) {
                    pson config;
                    config["user"] = (const char *) user;
                    config["device"] = (const char *) device;
                    config["credential"] = (const char *) device_credential;
                    pson_spiffs_encoder encoder(configFile);
                    encoder.encode(config);
                    configFile.close();
                    THINGER_DEBUG("_CONFIG", "Done!");
                } else {
                    THINGER_DEBUG("_CONFIG", "Failed to open config file for writing!");
                }
                SPIFFS.end();
            }
        }

        return true;
    }

private:
#ifndef _DISABLE_TLS_
    WiFiClientSecure client_;
#else
    WiFiClient client_;
#endif
    char user[40];
    char device[40];
    char device_credential[40];
};

#endif