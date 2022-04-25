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

#include <DNSServer.h>
#include <WiFiManager.h>

#define CONFIG_FILE "/config.pson"

#ifndef THINGER_DEVICE_SSID
    #define THINGER_DEVICE_SSID "Thinger-Device"
#endif

#ifndef THINGER_DEVICE_SSID_PSWD
    #define THINGER_DEVICE_SSID_PSWD "thinger.io"
#endif

#define MAX_ADDITIONAL_PARAMETERS 5

template <class ThingerDevice>
class ThingerWebConfig : public ThingerDevice {

public:
    ThingerWebConfig(const char* user="", const char* device="", const char* credential="") :
        ThingerDevice(user_, device_, credential_), 
        custom_params_(0),
        config_callback_(NULL), 
        wifi_callback_(NULL), 
        captive_portal_callback_(NULL)
    {
        strcpy(user_, user);
        strcpy(device_, device);
        strcpy(credential_, credential);
    }

    ~ThingerWebConfig(){

    }

    virtual bool clean_credentials() = 0;

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

protected:

    virtual bool load_configuration(pson& configuration) = 0;
    virtual bool save_configuration(pson& configuration) = 0;

    bool connect_network() override{

        // read current thinger.io credentials from file system
        pson config;
        bool thinger_config = load_configuration(config);

        if(thinger_config){
            THINGER_DEBUG("_CONFIG", "Config File Decoded!");
            if(strlen(user_)==0)        strcpy(user_, config["user"]);
            if(strlen(device_)==0)      strcpy(device_, config["device"]);
            if(strlen(credential_)==0)  strcpy(credential_, config["credential"]);

            THINGER_DEBUG_VALUE("_CONFIG", "User: ", user_);
            THINGER_DEBUG_VALUE("_CONFIG", "Device: ", device_);
            THINGER_DEBUG_VALUE("_CONFIG", "Credential: ", credential_);

            if(config_callback_!=NULL) config_callback_(config);
        }

        WiFiManager wifiManager;
#ifdef _DEBUG_
            wifiManager.setDebugOutput(true);
#else
            wifiManager.setDebugOutput(false);
#endif

        // if credentials and wifi already configured, try to connect to wifi
        if(thinger_config && wifiManager.getWiFiIsSaved()){
            THINGER_DEBUG("_CONFIG", "Connecting using previous configuration...");
            return ThingerDevice::connect_network();
        // don't have credentials or Wifi, initiate config portal
        }else{
            THINGER_DEBUG("_CONFIG", "Starting Webconfig...");

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

            save_configuration(config);

            if(config_callback_!=NULL) config_callback_(config);
            if(wifi_callback_!=NULL) wifi_callback_(wifiConnected);
            return WiFi.status() == WL_CONNECTED;
        }
    }

private:
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