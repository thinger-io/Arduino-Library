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
// Select the filesystem used by the deployed firmware, only for migration.
// SPIFFS is the upstream ESP32 default. Never format during migration.
#ifdef THINGER_ESP32_CONFIG_USE_LITTLEFS
#include <LittleFS.h>
#else
#include <SPIFFS.h>
#endif
#include "ThingerESP32.h"
#include "ThingerWebConfig.h"
#include "ThingerESP32NVS.h"

class ThingerESP32WebConfig : public ThingerWebConfig<ThingerESP32> {
public:
    ThingerESP32WebConfig(const char* user="", const char* device="", const char* credential="") :
        ThingerWebConfig<ThingerESP32>(user, device, credential) {}

    bool clean_credentials() override {
        // A committed empty-state record also blocks stale file reimports.
        // Preserve the existing API's scope: WiFiManager's credentials are separate.
        storage_error_ = !thinger_config::nvs_config::clear();
        return !storage_error_;
    }

protected:
    bool configuration_storage_error() const override { return storage_error_ || !this->wifi_storage_ok(); }

    bool prepare_wifi_configuration() override { return this->restore_wifi_networks(); }

    bool has_wifi_configuration(WiFiManager& manager) override {
        return this->has_wifi_networks() || (!this->wifi_registry_exists_ && manager.getWiFiIsSaved());
    }

    bool save_portal_wifi_configuration(WiFiManager& manager) override {
        const String ssid = manager.getWiFiSSID(true);
        const String password = manager.getWiFiPass(true);
        this->add_wifi(ssid.c_str(), password.c_str());
        return this->wifi_storage_ok();
    }

    bool save_configuration(pson& config) override {
        storage_error_ = !thinger_config::nvs_config::save(config);
        if(storage_error_) THINGER_DEBUG("_CONFIG", "Cannot save NVS configuration!");
        return !storage_error_;
    }

    bool load_configuration(pson& config) override {
        using store = thinger_config::nvs_config;
        storage_error_ = false;
        const store::result result = store::load(config);
        if(result == store::result::ready) return true;
        if(result == store::result::cleared) return false;
        if(result == store::result::error) {
            storage_error_ = true;
            THINGER_DEBUG("_CONFIG", "Cannot read NVS configuration; legacy import blocked!");
            return false;
        }
        // Only a missing NVS record may trigger the one-time legacy import.
#ifdef THINGER_ESP32_CONFIG_SKIP_LEGACY_MIGRATION
        // New production devices may omit the legacy filesystem entirely.
        return false;
#else
        return migrate_legacy(config);
#endif
    }

private:
    bool storage_error_ = false;

    bool migrate_legacy(pson& config) {
#ifdef THINGER_ESP32_CONFIG_USE_LITTLEFS
        auto& filesystem = LittleFS;
#else
        auto& filesystem = SPIFFS;
#endif
        if(!filesystem.begin(false)) {
            // Distinguish an unreadable legacy volume from an absent file.
            // Explicit clean_credentials() can initialize an NVS-only device.
            storage_error_ = true;
            THINGER_DEBUG("_CONFIG", "Cannot mount legacy FS; configuration left intact!");
            return false;
        }
        if(!filesystem.exists(CONFIG_FILE)) {
            filesystem.end();
            return false;
        }
        File file = filesystem.open(CONFIG_FILE, "r");
        bool migrated = false;
        if(file) {
            const size_t size = file.size();
            if(size && size <= THINGER_NVS_CONFIG_MAX_SIZE) {
                std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[size]);
                if(bytes && file.readBytes(reinterpret_cast<char*>(bytes.get()), size) == size) {
                    // save_legacy validates PSON and verifies the committed bytes.
                    // Reload from NVS before using the migrated configuration.
                    migrated = thinger_config::nvs_config::save_legacy(bytes.get(), size) &&
                               thinger_config::nvs_config::load(config) == thinger_config::nvs_config::result::ready;
                }
            }
            file.close();
        }
        if(migrated && !filesystem.remove(CONFIG_FILE)) {
            // NVS is already authoritative, even if cleanup could not finish.
            THINGER_DEBUG("_CONFIG", "NVS migration complete; legacy file cleanup failed!");
        }
        filesystem.end();
        storage_error_ = !migrated;
        return migrated;
    }
};

#endif
