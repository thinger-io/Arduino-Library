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

#ifndef THINGER_ESP32_WIFI_STORE_H
#define THINGER_ESP32_WIFI_STORE_H

#include <string>
#include <vector>
#include "ThingerESP32NVS.h"

namespace thinger_config {

struct wifi_network {
    std::string ssid;
    std::string password;
};

class wifi_store {
public:
    using networks = std::vector<wifi_network>;

    // Match the accepted credentials of the ESP32 WiFiMulti core (2.x/3.x).
    static bool valid(const char* ssid, const char* password) {
        return ssid && *ssid && strlen(ssid) <= 31 && (!password || strlen(password) <= 63);
    }

    static bool load(networks& output, bool* record_exists = nullptr) {
        protoson::pson data;
        const auto result = nvs_config::load(data, "wifi");
        if(result == nvs_config::result::error) return false;
        networks loaded;
        if(result == nvs_config::result::ready) {
            if(!data["version"].is_number() || (int)data["version"] != 1 || !data["networks"].is_array()) return false;
            protoson::pson_array& entries = data["networks"];
            if(entries.size() > 255) return false;
            for(auto it = entries.begin(); it.valid(); it.next()) {
                auto& item = it.item();
                if(!item.is_object() || !item["ssid"].is_string() ||
                   !(item["password"].is_string() || item["password"].get_type() == protoson::pson::empty_string)) return false;
                const char* ssid = item["ssid"];
                const char* password = item["password"];
                if(!valid(ssid, password)) return false;
                for(const auto& network : loaded) if(network.ssid == ssid) return false;
                loaded.push_back({ssid, password});
            }
        }
        output.swap(loaded);
        if(record_exists) *record_exists = result != nvs_config::result::missing;
        return true;
    }

    static bool save(const networks& input) {
        protoson::pson data;
        data["version"] = 1;
        protoson::pson_array& entries = data["networks"];
        if(input.size() > 255) return false;
        for(const auto& network : input) {
            if(!valid(network.ssid.c_str(), network.password.c_str())) return false;
            auto* item = entries.create_item();
            if(!item) return false;
            (*item)["ssid"] = network.ssid.c_str();
            (*item)["password"] = network.password.c_str();
        }
        return entries.size() == input.size() && nvs_config::save(data, "wifi");
    }
};

} // namespace thinger_config

#endif
