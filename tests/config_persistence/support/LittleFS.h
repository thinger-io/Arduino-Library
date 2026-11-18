#pragma once
#include "Arduino.h"
#include <map>
#include <memory>
#include <vector>
struct TestFS;
extern TestFS LittleFS;
struct FSInfo {};
struct LittleFSConfig {explicit LittleFSConfig(bool format=true):autoFormat(format){} bool autoFormat;};
struct File {
    std::string path;bool opened;
    operator bool()const{return opened;}
    size_t write(const uint8_t*,size_t);
    size_t readBytes(char*,size_t){return 0;}
    void close(){}
    void flush(){}
};
namespace littlefs_impl {struct LittleFSImpl{LittleFSImpl(unsigned,unsigned,unsigned,unsigned,unsigned){}};}
namespace fs {using FSImplPtr=std::shared_ptr<littlefs_impl::LittleFSImpl>;}
struct TestFS {
    TestFS()=default;
    explicit TestFS(fs::FSImplPtr){}
    std::map<std::string,std::vector<uint8_t>> files;
    bool mount_ok=true,open_ok=true,rename_ok=true;
    size_t budget=100000;int ends=0;bool mounted=false,auto_format=true,formatted_on_begin=false;
    bool begin(){LittleFS.formatted_on_begin=!LittleFS.mount_ok && auto_format;mounted=LittleFS.mount_ok;return mounted;}
    void end(){++ends;mounted=false;}
    bool info(FSInfo&){return mounted;}
    bool setConfig(const LittleFSConfig& c){if(mounted)return false;auto_format=c.autoFormat;return true;}
    bool exists(const char* p){return LittleFS.files.count(p)>0;}
    bool remove(const char* p){return LittleFS.files.erase(p)>0;}
    bool rename(const char* from,const char* to){
        if(!LittleFS.rename_ok)return false;
        LittleFS.files[to]=LittleFS.files[from];LittleFS.files.erase(from);return true;
    }
    File open(const char* p,const char* mode){
        if(LittleFS.open_ok && mode[0]=='w')LittleFS.files[p].clear();
        return File{p,LittleFS.open_ok};
    }
};
using FS=TestFS;
TestFS LittleFS;
size_t File::write(const uint8_t* p,size_t n){
    size_t wrote=std::min(n,LittleFS.budget);
    LittleFS.files[path].insert(LittleFS.files[path].end(),p,p+wrote);
    LittleFS.budget-=wrote;return wrote;
}

