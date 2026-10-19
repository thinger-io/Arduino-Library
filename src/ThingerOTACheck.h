#ifndef THINGER_OTA_CHECK_H
#define THINGER_OTA_CHECK_H

#include "thinger/pson.h"

// Define THINGER_OTA_CHECK(FIELD) before including ThingerESP32OTA.h.
// FIELD accepts two string literals: the field name and its expected value.
#ifdef THINGER_OTA_CHECK
namespace thinger_ota_check {

constexpr size_t string_length(const char* text) {
    return *text ? 1 + string_length(text + 1) : 0;
}

#define THINGER_OTA_CHECK_VALIDATE(key, value) \
    static_assert(sizeof(key) > 1 && sizeof(key) <= 65, "OTA check keys must contain 1 to 64 bytes"); \
    static_assert(sizeof(value) <= 257, "OTA check values must contain at most 256 bytes"); \
    static_assert(string_length(key) == sizeof(key) - 1, "OTA check keys cannot contain NUL bytes"); \
    static_assert(string_length(value) == sizeof(value) - 1, "OTA check values cannot contain NUL bytes");
THINGER_OTA_CHECK(THINGER_OTA_CHECK_VALIDATE)
#undef THINGER_OTA_CHECK_VALIDATE

#define THINGER_OTA_CHECK_COUNT(key, value) + 1
static constexpr size_t field_count = 0 THINGER_OTA_CHECK(THINGER_OTA_CHECK_COUNT);
#undef THINGER_OTA_CHECK_COUNT
static_assert(field_count > 0 && field_count <= 32, "OTA check requires 1 to 32 fields");

// Version 1: header, NUL-separated UTF-8 key/value pairs, an empty key, footer.
// The resource reads this same array, keeping it reachable through the linker.
// A template static member gives C++11 projects one linked copy across source files.
#define THINGER_OTA_CHECK_PACK(key, value) key "\0" value "\0"
template<typename Tag = void>
struct definition {
    static constexpr char manifest[] =
        "@THINGER_OTA_CHECK:1@\0"
        THINGER_OTA_CHECK(THINGER_OTA_CHECK_PACK)
        "\0@END_THINGER_OTA_CHECK@";
};
template<typename Tag>
constexpr char definition<Tag>::manifest[];
#undef THINGER_OTA_CHECK_PACK
static constexpr size_t header_size = sizeof("@THINGER_OTA_CHECK:1@");
static_assert(sizeof(definition<>::manifest) - header_size - sizeof("@END_THINGER_OTA_CHECK@") <= 4096,
    "OTA check payload cannot exceed 4096 bytes");

constexpr bool equal(const char* left, const char* right) {
    return *left == *right && (!*left || equal(left + 1, right + 1));
}

constexpr const char* next_field(const char* field) {
    return field + string_length(field) + 1;
}

constexpr const char* next_entry(const char* key) {
    return next_field(next_field(key));
}

constexpr bool distinct_from(const char* key, const char* other) {
    return !*other || (!equal(key, other) && distinct_from(key, next_entry(other)));
}

constexpr bool unique_keys(const char* key) {
    return !*key || (distinct_from(key, next_entry(key)) && unique_keys(next_entry(key)));
}

static_assert(unique_keys(definition<>::manifest + header_size), "OTA check keys must be unique");

inline void fill_output(protoson::pson& out) {
    const char* key = definition<>::manifest + header_size;
    while (*key) {
        const char* value = key + strlen(key) + 1;
        out[key] = value;
        key = value + strlen(value) + 1;
    }
}

} // namespace thinger_ota_check
#endif
#endif
