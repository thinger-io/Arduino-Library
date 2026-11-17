#pragma once
#include "Arduino.h"
#include <map>
#include <vector>
struct TestFS;
extern TestFS LittleFS;
struct File {
    std::string path;bool opened;
    operator bool()const{return opened;}
    size_t write(const uint8_t*,size_t);
    size_t readBytes(char*,size_t){return 0;}
    void close(){}
    void flush(){}
};
struct TestFS {
    std::map<std::string,std::vector<uint8_t>> files;
    bool mount_ok=true,open_ok=true,rename_ok=true;
    size_t budget=100000;int ends=0;
    bool begin(){return mount_ok;}void end(){++ends;}
    bool exists(const char* p){return files.count(p)>0;}
    bool remove(const char* p){return files.erase(p)>0;}
    bool rename(const char* from,const char* to){
        if(!rename_ok)return false;
        files[to]=files[from];files.erase(from);return true;
    }
    File open(const char* p,const char* mode){
        if(open_ok && mode[0]=='w')files[p].clear();
        return File{p,open_ok};
    }
};
TestFS LittleFS;
size_t File::write(const uint8_t* p,size_t n){
    size_t wrote=std::min(n,LittleFS.budget);
    LittleFS.files[path].insert(LittleFS.files[path].end(),p,p+wrote);
    LittleFS.budget-=wrote;return wrote;
}
