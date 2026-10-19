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

#ifndef THINGER_ESP32_NVS_H
#define THINGER_ESP32_NVS_H

#include <nvs.h>
#include <memory>
#include <new>
#include "thinger/pson.h"

// Maximum PSON payload, excluding the record header. Override before including.
#ifndef THINGER_NVS_CONFIG_MAX_SIZE
#define THINGER_NVS_CONFIG_MAX_SIZE 16384
#endif

namespace thinger_config {

class buffer_decoder : public protoson::pson_decoder {
public:
    buffer_decoder(const uint8_t* data, size_t size) : data_(data), size_(size) {}
protected:
    bool read(void* buffer, size_t size) override {
        if(size > size_ - bytes_read()) return false;
        memcpy(buffer, data_ + bytes_read(), size);
        return protoson::pson_decoder::read(buffer, size);
    }
private:
    const uint8_t* data_;
    size_t size_;
};

class buffer_encoder : public protoson::pson_encoder {
public:
    buffer_encoder(uint8_t* data, size_t size) : data_(data), size_(size) {}
    bool valid() const { return valid_; }
protected:
    bool write(const void* buffer, size_t size) override {
        if(size > size_ - bytes_written()) {
            valid_ = false;
            return false;
        }
        memcpy(data_ + bytes_written(), buffer, size);
        return protoson::pson_encoder::write(buffer, size);
    }
private:
    uint8_t* data_;
    size_t size_;
    bool valid_ = true;
};

// One NVS blob is the commit unit: header and the complete PSON object are
// replaced together. A cleared record prevents reimporting stale legacy files.
class nvs_config {
public:
    enum class result { missing, ready, cleared, error };

    static bool decode(const uint8_t* bytes, size_t size, protoson::pson& config) {
        if(!bytes || !size || size > THINGER_NVS_CONFIG_MAX_SIZE) return false;
        protoson::pson decoded;
        buffer_decoder decoder(bytes, size);
        if(!decoder.decode(decoded) || decoder.bytes_read() != size || !decoded.is_object()) return false;
        protoson::pson::swap(decoded, config);
        return true;
    }

    static result load(protoson::pson& config, const char* key = "config") {
        handle storage;
        esp_err_t err = nvs_open("thinger", NVS_READONLY, &storage.value);
        if(err == ESP_ERR_NVS_NOT_FOUND) return result::missing;
        if(err != ESP_OK) return result::error;
        storage.open = true;
        size_t size = 0;
        err = nvs_get_blob(storage.value, key, nullptr, &size);
        if(err == ESP_ERR_NVS_NOT_FOUND) return result::missing;
        if(err != ESP_OK || size < header_size || size - header_size > THINGER_NVS_CONFIG_MAX_SIZE) return result::error;
        std::unique_ptr<uint8_t[]> record(new(std::nothrow) uint8_t[size]);
        if(!record || nvs_get_blob(storage.value, key, record.get(), &size) != ESP_OK) return result::error;
        if(memcmp(record.get(), "TCFG", 4) || record[4] != 1 || record[6] || record[7]) return result::error;
        if(record[5] == 0) return size == header_size ? result::cleared : result::error;
        if(record[5] != 1 || !decode(record.get() + header_size, size - header_size, config)) return result::error;
        return result::ready;
    }

    static bool save(protoson::pson& config, const char* key = "config") {
        if(!config.is_object()) return false;
        protoson::pson_encoder counter;
        counter.encode(config);
        const size_t size = counter.bytes_written();
        if(!size || size > THINGER_NVS_CONFIG_MAX_SIZE) return false;
        std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[size]);
        if(!bytes) return false;
        buffer_encoder encoder(bytes.get(), size);
        encoder.encode(config);
        return encoder.valid() && encoder.bytes_written() == size && save_legacy(bytes.get(), size, key);
    }

    // Keep the original PSON bytes, including fields unknown to this firmware.
    static bool save_legacy(const uint8_t* bytes, size_t size, const char* key = "config") {
        protoson::pson config;
        return decode(bytes, size, config) && write_record(bytes, size, 1, key);
    }

    static bool clear(const char* key = "config") { return write_record(nullptr, 0, 0, key); }

private:
    enum { header_size = 8 };
    struct handle {
        nvs_handle_t value = 0;
        bool open = false;
        ~handle() { if(open) nvs_close(value); }
    };

    static bool write_record(const uint8_t* bytes, size_t size, uint8_t state, const char* key) {
        if(size > THINGER_NVS_CONFIG_MAX_SIZE) return false;
        const size_t total = header_size + size;
        std::unique_ptr<uint8_t[]> record(new(std::nothrow) uint8_t[total]);
        std::unique_ptr<uint8_t[]> check(new(std::nothrow) uint8_t[total]);
        if(!record || !check) return false;
        const uint8_t header[header_size] = {'T', 'C', 'F', 'G', 1, state, 0, 0};
        memcpy(record.get(), header, header_size);
        if(size) memcpy(record.get() + header_size, bytes, size);
        {
            handle storage;
            if(nvs_open("thinger", NVS_READWRITE, &storage.value) != ESP_OK) return false;
            storage.open = true;
            if(nvs_set_blob(storage.value, key, record.get(), total) != ESP_OK ||
               nvs_commit(storage.value) != ESP_OK) return false;
        }
        handle storage;
        if(nvs_open("thinger", NVS_READONLY, &storage.value) != ESP_OK) return false;
        storage.open = true;
        size_t read = total;
        return nvs_get_blob(storage.value, key, check.get(), &read) == ESP_OK &&
               read == total && memcmp(record.get(), check.get(), total) == 0;
    }
};

} // namespace thinger_config

#endif
