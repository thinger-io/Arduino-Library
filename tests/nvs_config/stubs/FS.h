#pragma once
#include <vector>
#include <stdint.h>
#include <string.h>
#include <algorithm>
class File {
public:
    File(std::vector<uint8_t>* bytes = nullptr) : bytes_(bytes) {}
    explicit operator bool() const { return bytes_ != nullptr; }
    size_t size() const { return bytes_->size(); }
    size_t readBytes(char* out, size_t size) {
        size = std::min(size, bytes_->size() - offset_);
        memcpy(out, bytes_->data() + offset_, size);
        offset_ += size;
        return size;
    }
    void close() { bytes_ = nullptr; }
private:
    std::vector<uint8_t>* bytes_;
    size_t offset_ = 0;
};
struct FakeFS {
    bool mount_ok = true, present = false, remove_ok = true;
    int mounts = 0, unmounts = 0, removals = 0;
    std::vector<uint8_t> bytes;
    bool begin(bool format) { if(format) throw "migration must never format"; ++mounts; return mount_ok; }
    bool exists(const char*) { return present; }
    File open(const char*, const char* mode) { if(strcmp(mode, "r")) throw "legacy FS must be read-only"; return File(present ? &bytes : nullptr); }
    bool remove(const char*) { ++removals; if(!remove_ok) return false; present = false; return true; }
    void end() { ++unmounts; }
};
