#ifndef THINGER_UPDATE_ERROR_H
#define THINGER_UPDATE_ERROR_H

#include <Print.h>

// helper class to get message from update error 
class UpdateErrorGetter : public Print{
public:
    UpdateErrorGetter() {}
    virtual ~UpdateErrorGetter(){}

    virtual size_t write(uint8_t value){
        if(index_<BUFFER_SIZE-1){
            buffer_[index_++] = value;
            return 1;
        }else{
            return 0;
        }
    }

    const char* get_error(){
        buffer_[index_]=0;
        return buffer_;
    }

private:
    static const size_t BUFFER_SIZE = 128;
    char buffer_[BUFFER_SIZE];
    size_t index_ = 0;
};

#endif
