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

#define MAX_ADDITIONAL_PARAMETERS 5


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
    ThingerWebConfig(const char* user="", const char* device="", const char* credential="") :
        ThingerClient(client_, user_, device_, credential_), custom_params_(0),
        config_callback_(NULL), wifi_callback_(NULL), captive_portal_callback_(NULL)
    {
        strcpy(user_, user);
        strcpy(device_, device);
        strcpy(credential_, credential);
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


#ifndef _DISABLE_TLS_
protected:
    virtual bool connect_socket(){

        // since CORE 2.5.0, now it is used BearSSL by default
#ifndef _VALIDATE_SSL_CERTIFICATE_
        client_.setInsecure();
        THINGER_DEBUG("SSL/TLS", "Warning: use #define _VALIDATE_SSL_CERTIFICATE_ if certificate validation is required")
#else
        client_.setFingerprint(THINGER_TLS_FINGERPRINT);
        THINGER_DEBUG_VALUE("SSL/TLS", "SHA-1 certificate fingerprint: ", THINGER_TLS_FINGERPRINT)
#endif
        return client_.connect(get_host(), THINGER_SSL_PORT);
    }

    virtual bool secure_connection(){
        return true;
    }
#endif


protected:

    virtual bool network_connected(){
        return WiFi.status() == WL_CONNECTED && !(WiFi.localIP() == INADDR_NONE);
    }

    virtual bool connect_network(){
        bool thinger_config = false;

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
                    if(strlen(user_)==0)        strcpy(user_, config["user"]);
                    if(strlen(device_)==0)      strcpy(device_, config["device"]);
                    if(strlen(credential_)==0)  strcpy(credential_, config["credential"]);

                    thinger_config = true;

                    THINGER_DEBUG_VALUE("_CONFIG", "User: ", user_);
                    THINGER_DEBUG_VALUE("_CONFIG", "Device: ", device_);
                    THINGER_DEBUG_VALUE("_CONFIG", "Credential: ", credential_);

                    if(config_callback_!=NULL) config_callback_(config);
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
        if(thinger_config && WiFi.SSID() && !WiFi.SSID().length()==0){
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

            WiFiManagerParameter user_parameter("user", "User Id", user_, 40);
            WiFiManagerParameter device_parameter("device", "Device Id", device_, 40);
            WiFiManagerParameter credential_parameter("credential", "Device Credential", credential_, 40);

            // add credentials parameters if not set at startup
            if(strlen(user_)==0)        wifiManager.addParameter(&user_parameter);
            if(strlen(device_)==0)      wifiManager.addParameter(&device_parameter);
            if(strlen(credential_)==0)  wifiManager.addParameter(&credential_parameter);

            // add custom parameters
            for(int i=0; i<custom_params_; i++){
                wifiManager.addParameter(parameters_[i]);
            }

            if(captive_portal_callback_!=NULL) captive_portal_callback_(wifiManager);

            bool wifiConnected = wifiManager.startConfigPortal(THINGER_DEVICE_SSID, THINGER_DEVICE_SSID_PSWD);

            // store new values (if it was empty)
            if(strlen(user_)==0)        strcpy(user_, user_parameter.getValue());
            if(strlen(device_)==0)      strcpy(device_, device_parameter.getValue());
            if(strlen(credential_)==0)  strcpy(credential_, credential_parameter.getValue());

            pson config;
            config["user"] = (const char *) user_;
            config["device"] = (const char *) device_;
            config["credential"] = (const char *) credential_;

            for(int i=0; i<custom_params_; i++){
                config[parameters_[i]->getID()] = parameters_[i]->getValue();
            }

            THINGER_DEBUG("_CONFIG", "Updating Device Info...");
            if (SPIFFS.begin()) {
                File configFile = SPIFFS.open(CONFIG_FILE, "w");
                if (configFile) {
                    pson_spiffs_encoder encoder(configFile);
                    encoder.encode(config);
                    configFile.close();
                    THINGER_DEBUG("_CONFIG", "Done!");
                } else {
                    THINGER_DEBUG("_CONFIG", "Failed to open config file for writing!");
                }
                SPIFFS.end();
            }

            if(config_callback_!=NULL) config_callback_(config);
            if(wifi_callback_!=NULL) wifi_callback_(wifiConnected);
        }

        return WiFi.status() == WL_CONNECTED;
    }

public:
    bool add_setup_parameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom=""){
        if(custom_params_<MAX_ADDITIONAL_PARAMETERS){
            parameters_[custom_params_++]= new WiFiManagerParameter(id, placeholder, defaultValue, length, custom);
            return true;
        }
       return false;
    }

    void set_on_config_callback(void (*callback)(pson &)){
        config_callback_ = callback;
    }

    void set_on_wifi_config(void (*callback)(bool)){
        wifi_callback_ = callback;
    }

    void set_on_captive_portal_run(void (*callback)(WiFiManager&)){
        captive_portal_callback_ = callback;
    }

    void set_user(const char*user){
        strcpy(user_, user);
    }

    void set_device(const char*device){
        strcpy(device_, device);
    }

    void set_credential(const char*credential){
        strcpy(credential_, credential);
    }

private:
#ifndef _DISABLE_TLS_
    WiFiClientSecure client_;
#else
    WiFiClient client_;
#endif
    char user_[40];
    char device_[40];
    char credential_[40];
    WiFiManagerParameter *parameters_[MAX_ADDITIONAL_PARAMETERS] ;
    int custom_params_;
    void (*config_callback_)(pson &);
    void (*wifi_callback_)(bool);
    void (*captive_portal_callback_)(WiFiManager& manager);

};

#endif