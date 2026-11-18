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

// failed connections to the saved network before opening the config portal again (0 to disable it)
#ifndef THINGER_WEBCONFIG_ATTEMPTS
    #define THINGER_WEBCONFIG_ATTEMPTS 3
#endif

// seconds the config portal is open after failing to connect to the saved network
#ifndef THINGER_WEBCONFIG_PORTAL_TIMEOUT
    #define THINGER_WEBCONFIG_PORTAL_TIMEOUT 180
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
        copy_credential(user_, user);
        copy_credential(device_, device);
        copy_credential(credential_, credential);
        fixed_user_ = user_[0];
        fixed_device_ = device_[0];
        fixed_secret_ = credential_[0];
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
        if(copy_credential(user_, user)) fixed_user_ = user_[0];
    }

    void set_device(const char*device){
        if(copy_credential(device_, device)) fixed_device_ = device_[0];
    }

    void set_credential(const char*credential){
        if(copy_credential(credential_, credential)) fixed_secret_ = credential_[0];
    }

    // Check physical consent (for example, a button) in authorize, and provision
    // a unique WPA password for this device. Passing nullptr disables recovery.
    void set_credential_recovery(bool (*authorize)(), const char* portal_password){
        authorize_recovery_ = nullptr;
        recovery_pending_ = false;
        memset(recovery_password_, 0, sizeof(recovery_password_));
        size_t length = portal_password ? strlen(portal_password) : 0;
        if(authorize && length >= 8 && length < sizeof(recovery_password_)){
            memcpy(recovery_password_, portal_password, length + 1);
            authorize_recovery_ = authorize;
        }
    }

protected:

    virtual bool load_configuration(pson& configuration) = 0;
    virtual bool save_configuration(pson& configuration) = 0;

    // the config portal requires the network before continuing, so it waits for the connection
    typename ThingerDevice::network_status connect_network_step() override{
        return connect_network() ? ThingerDevice::network_status::connected : ThingerDevice::network_status::failed;
    }

    void credentials_rejected() override{
        recovery_pending_ = authorize_recovery_ && !(fixed_user_ && fixed_device_ && fixed_secret_);
    }

    bool network_connected() override{
        return !recovery_pending_ && ThingerDevice::network_connected();
    }

    bool connect_network() override{

        if(recovery_pending_){
            recovery_pending_ = false;
            if(authorize_recovery_ && authorize_recovery_()) return recover_credentials();
            if(ThingerDevice::network_connected()) return true;
        }

        // read current thinger.io credentials from file system
        pson config;
        bool thinger_config = load_configuration(config);

        if(thinger_config){
            THINGER_DEBUG("_CONFIG", "Config File Decoded!");
            const char* saved_user = config["user"];
            const char* saved_device = config["device"];
            const char* saved_credential = config["credential"];
            thinger_config = (user_[0] || filled_credential(saved_user)) &&
                             (device_[0] || filled_credential(saved_device)) &&
                             (credential_[0] || filled_credential(saved_credential));
            if(thinger_config){
                if(!user_[0]) copy_credential(user_, saved_user);
                if(!device_[0]) copy_credential(device_, saved_device);
                if(!credential_[0]) copy_credential(credential_, saved_credential);
            }

            THINGER_DEBUG_VALUE("_CONFIG", "User: ", user_);
            THINGER_DEBUG_VALUE("_CONFIG", "Device: ", device_);
            THINGER_DEBUG_VALUE("_CONFIG", "Credential: ", credential_);

            if(thinger_config && config_callback_!=NULL) config_callback_(config);
        }

        WiFiManager wifiManager;
#ifdef _DEBUG_
            wifiManager.setDebugOutput(true);
#else
            wifiManager.setDebugOutput(false);
#endif

#ifdef ESP32
        // WiFiManager reads the stored WiFi from the driver, so it must be initialized (otherwise the result is random)
        WiFi.mode(WIFI_STA);
#endif

        // credentials set in the constructor do not need the config portal, unless it has custom parameters
        bool thinger_credentials = strlen(user_)>0 && strlen(device_)>0 && strlen(credential_)>0 && custom_params_==0;

        // if credentials and wifi already configured, try to connect to wifi
        if((thinger_config || thinger_credentials) && wifiManager.getWiFiIsSaved()){
            THINGER_DEBUG("_CONFIG", "Connecting using previous configuration...");
            if(ThingerDevice::connect_network()){
                failed_connections_ = 0;
                return true;
            }
            // the device may have been moved to another place: open the config portal for a while
            if(THINGER_WEBCONFIG_ATTEMPTS==0 || ++failed_connections_ < THINGER_WEBCONFIG_ATTEMPTS) return false;
            failed_connections_ = 0;
            THINGER_DEBUG("_CONFIG", "Cannot connect to the saved network, starting Webconfig...");
            wifiManager.setConfigPortalTimeout(THINGER_WEBCONFIG_PORTAL_TIMEOUT);
            // show the current values of the custom parameters, so they are not reset
            for(int i=0; i<custom_params_; i++){
                if(config[parameters_[i]->getID()].is_string()){
                    parameters_[i]->setValue(config[parameters_[i]->getID()], parameters_[i]->getValueLength());
                }
            }
            // on timeout, keep the configuration and the error, and try the saved network again
            return run_config_portal(wifiManager, false);
        // don't have credentials or Wifi, initiate config portal
        }else{
            THINGER_DEBUG("_CONFIG", "Starting Webconfig...");
            if(run_config_portal(wifiManager, true)) return true;
            // the config portal finished without configuring the WiFi
            this->set_error(ThingerClient::NETWORK_NOT_CONFIGURED);
            return false;
        }
    }

    // runs the config portal and saves the configuration (if not connected, only when save_always)
    bool run_config_portal(WiFiManager& wifiManager, bool save_always){
        WiFiManagerParameter user_parameter("user", "User Id", user_, credential_max_length);
        WiFiManagerParameter device_parameter("device", "Device Id", device_, credential_max_length);
        WiFiManagerParameter credential_parameter("credential", "Device Credential", credential_, credential_max_length);

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
        if(!wifiConnected && !save_always){
            if(wifi_callback_!=NULL) wifi_callback_(wifiConnected);
            return false;
        }

        // Validate the complete update before modifying or saving credentials.
        if((!user_[0] && !filled_credential(user_parameter.getValue())) ||
           (!device_[0] && !filled_credential(device_parameter.getValue())) ||
           (!credential_[0] && !filled_credential(credential_parameter.getValue()))){
            if(wifi_callback_!=NULL) wifi_callback_(false);
            return false;
        }
        if(!user_[0]) copy_credential(user_, user_parameter.getValue());
        if(!device_[0]) copy_credential(device_, device_parameter.getValue());
        if(!credential_[0]) copy_credential(credential_, credential_parameter.getValue());

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

private:
    static const size_t credential_max_length = 40;

    // Empty constructor defaults request provisioning; persisted fields must be complete.
    static bool filled_credential(const char* value){
        return value && *value && valid_credential(value);
    }

    static bool valid_credential(const char* value){
        if(value){
            for(size_t i=0; i<=credential_max_length; ++i){
                if(value[i]=='\0') return true;
            }
        }
        THINGER_DEBUG("_CONFIG", "Invalid credential length (maximum 40 characters)");
        return false;
    }

    static bool copy_credential(char* destination, const char* value){
        if(!valid_credential(value)) return false;
        memmove(destination, value, strlen(value) + 1);
        return true;
    }

    bool recover_credentials(){
        pson config;
        if(!load_configuration(config)) return false;
        const char* previous_user = user_[0] ? user_ : static_cast<const char*>(config["user"]);
        const char* previous_device = device_[0] ? device_ : static_cast<const char*>(config["device"]);
        const char* previous_secret = credential_[0] ? credential_ : static_cast<const char*>(config["credential"]);

        WiFiManager portal;
        portal.setConfigPortalTimeout(THINGER_WEBCONFIG_PORTAL_TIMEOUT);
        WiFiManagerParameter user("user", "User Id", previous_user, credential_max_length);
        WiFiManagerParameter device("device", "Device Id", previous_device, credential_max_length);
        WiFiManagerParameter secret("credential", "New device credential (blank keeps current)", "", credential_max_length);
        if(!fixed_user_) portal.addParameter(&user);
        if(!fixed_device_) portal.addParameter(&device);
        if(!fixed_secret_) portal.addParameter(&secret);
        if(!portal.startConfigPortal(THINGER_DEVICE_SSID, recovery_password_)) return false;

        const char* next_user = fixed_user_ ? user_ : user.getValue();
        const char* next_device = fixed_device_ ? device_ : device.getValue();
        const char* next_secret = !fixed_secret_ && secret.getValue()[0] ? secret.getValue() : previous_secret;
        if(!filled_credential(next_user) || !filled_credential(next_device) || !filled_credential(next_secret)) return false;

        // A PSON string is allocated once. Replace the existing value while
        // preserving any application fields carried in the same object.
        pson u(next_user), d(next_device), c(next_secret);
        if(!u.is_string() || !d.is_string() || !c.is_string()) return false;
        pson::swap(u, config["user"]);
        pson::swap(d, config["device"]);
        pson::swap(c, config["credential"]);
        if(!save_configuration(config)) return false;
        copy_credential(user_, config["user"]);
        copy_credential(device_, config["device"]);
        copy_credential(credential_, config["credential"]);
        if(config_callback_) config_callback_(config);
        return true;
    }

    bool (*authorize_recovery_)() = nullptr;
    char recovery_password_[64] = {};
    bool recovery_pending_ = false;
    bool fixed_user_ = false, fixed_device_ = false, fixed_secret_ = false;

    char user_[credential_max_length + 1] = {};
    char device_[credential_max_length + 1] = {};
    char credential_[credential_max_length + 1] = {};
    WiFiManagerParameter *parameters_[MAX_ADDITIONAL_PARAMETERS] ;
    int custom_params_;
    uint8_t failed_connections_ = 0;
    void (*config_callback_)(pson &);
    void (*wifi_callback_)(bool);
    void (*captive_portal_callback_)(WiFiManager& manager);
    
};

#endif