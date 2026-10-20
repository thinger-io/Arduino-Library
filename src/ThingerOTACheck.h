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

#ifndef THINGER_OTA_CHECK_H
#define THINGER_OTA_CHECK_H

// OTA compatibility check (based on the proposal of @swinp in #60). Define it before including the OTA header:
//
//   #define THINGER_OTA_CHECK(FIELD) FIELD("PCB_MODEL", "SENSOR_I2C") FIELD("PCB_VERSION", "V2")
//
// The fields are embedded in the firmware binary, so the uploader (i.e., the VS Code extension) reads them from
// the new firmware, and compares them with the fields of the running firmware, reported in the $ota/options
// resource, asking for confirmation before updating a device with a different hardware.
#ifdef THINGER_OTA_CHECK

#include "thinger/pson.h"

namespace thinger_ota_check{

    constexpr size_t string_length(const char* text){
        return *text ? 1 + string_length(text + 1) : 0;
    }

    #define THINGER_OTA_CHECK_VALIDATE(key, value) \
        static_assert(sizeof(key) > 1 && sizeof(key) <= 33, "THINGER_OTA_CHECK keys must have 1 to 32 characters"); \
        static_assert(sizeof(value) <= 129, "THINGER_OTA_CHECK values must have at most 128 characters"); \
        static_assert(string_length(key) == sizeof(key) - 1, "THINGER_OTA_CHECK keys cannot contain NUL characters"); \
        static_assert(string_length(value) == sizeof(value) - 1, "THINGER_OTA_CHECK values cannot contain NUL characters");
    THINGER_OTA_CHECK(THINGER_OTA_CHECK_VALIDATE)
    #undef THINGER_OTA_CHECK_VALIDATE

    #define THINGER_OTA_CHECK_COUNT(key, value) + 1
    constexpr size_t field_count = 0 THINGER_OTA_CHECK(THINGER_OTA_CHECK_COUNT);
    #undef THINGER_OTA_CHECK_COUNT
    static_assert(field_count >= 1, "THINGER_OTA_CHECK requires at least one field");
    static_assert(field_count <= 16, "THINGER_OTA_CHECK supports up to 16 fields");

    // manifest version 1: header, NUL-separated key/value pairs, an empty key, and footer. It is a static member
    // of a template, so there is a single copy even if the OTA header is included from several source files
    #define THINGER_OTA_CHECK_PACK(key, value) key "\0" value "\0"
    template<typename T = void>
    struct manifest{
        static constexpr char data[] =
            "@THINGER_OTA_CHECK:1@\0"
            THINGER_OTA_CHECK(THINGER_OTA_CHECK_PACK)
            "\0@END_THINGER_OTA_CHECK@";
    };
    template<typename T>
    constexpr char manifest<T>::data[];
    #undef THINGER_OTA_CHECK_PACK

    constexpr size_t header_size = sizeof("@THINGER_OTA_CHECK:1@");
    static_assert(sizeof(manifest<>::data) - header_size - sizeof("@END_THINGER_OTA_CHECK@") <= 1024,
        "THINGER_OTA_CHECK fields cannot exceed 1024 bytes");

    constexpr bool equal(const char* left, const char* right){
        return *left == *right && (!*left || equal(left + 1, right + 1));
    }

    constexpr const char* next_entry(const char* key){
        return key + string_length(key) + 1 + string_length(key + string_length(key) + 1) + 1;
    }

    constexpr bool distinct_from(const char* key, const char* other){
        return !*other || (!equal(key, other) && distinct_from(key, next_entry(other)));
    }

    constexpr bool unique_keys(const char* key){
        return !*key || (distinct_from(key, next_entry(key)) && unique_keys(next_entry(key)));
    }

    static_assert(unique_keys(manifest<>::data + header_size), "THINGER_OTA_CHECK keys must be unique");

    // fills the fields from the manifest, so it is kept in the binary
    inline void fill(protoson::pson& out){
        const char* key = manifest<>::data + header_size;
        while(*key){
            const char* value = key + strlen(key) + 1;
            out[key] = value;
            key = value + strlen(value) + 1;
        }
    }

    // value of a field of this firmware, or nullptr if it does not exist
    inline const char* field(const char* name){
        const char* key = manifest<>::data + header_size;
        while(*key){
            const char* value = key + strlen(key) + 1;
            if(strcmp(key, name)==0) return value;
            key = value + strlen(value) + 1;
        }
        return nullptr;
    }

    // checks that the fields of a new firmware (sent in $ota/begin) are the same as the fields of this firmware
    inline bool matches(protoson::pson& fields){
        if(!fields.is_object()) return false;
        protoson::pson_object& object = fields;
        size_t count = 0;
        for(auto it = object.begin(); it.valid(); it.next()){
            protoson::pson& value = it.item().value();
            const char* own = field(it.item().name());
            if(own==nullptr || !value.is_string() || strcmp((const char*) value, own)!=0) return false;
            count++;
        }
        return count==field_count;
    }

}

#endif

#endif
