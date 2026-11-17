#pragma once
#include "Arduino.h"
#include <cstdarg>
#include <cstdio>
class Stream {
public:
    virtual ~Stream()=default;
    virtual size_t write(uint8_t)=0;
    virtual int available()=0;
    virtual int read()=0;
    virtual int peek()=0;
    virtual void flush()=0;
    size_t printf(const char* fmt,...){
        char b[512];va_list args;va_start(args,fmt);
        int n=vsnprintf(b,sizeof(b),fmt,args);va_end(args);
        for(int i=0;i<n && i<511;++i)write(b[i]);return n;
    }
    void println(const char* s){printf("%s\n",s);}
};
