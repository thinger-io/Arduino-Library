// The MIT License (MIT)
//
// Copyright (c) 2015 THINGER LTD
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

#ifndef THINGER_MESSAGE_HPP
#define THINGER_MESSAGE_HPP

#include "pson.h"

namespace thinger{

    class thinger_message{
    public:
        enum message_type{ MESSAGE = 1, KEEP_ALIVE = 2 };
        enum signal_flag { NONE=0, REQUEST_OK = 1, REQUEST_ERROR = 2 };
        enum fields{ STREAM_ID = 1, SIGNAL_FLAG = 2, THING_ID = 3, RESOURCE = 4, ACTION = 5, PSON = 6};

    public:

        thinger_message(thinger_message& other) : message_type_(other.message_type_), stream_id(other.stream_id), flag(REQUEST_OK), thing_id(NULL), resource(NULL), data(NULL), data_allocated(false){
        }

        thinger_message() : message_type_(MESSAGE), stream_id(0), flag(NONE), thing_id(NULL), resource(NULL), data(NULL), data_allocated(false)
        {}

        ~thinger_message(){
            protoson::pool.deallocate(thing_id);
            destroy(resource, protoson::pool);
            if(data_allocated){
                destroy(data, protoson::pool);
            }
        }

    private:
        message_type message_type_;
        uint32_t stream_id;
        signal_flag flag;
        char* thing_id;
        protoson::pson* resource;
        protoson::pson* data;
        bool data_allocated;

    public:

        message_type get_message_type(){
            return message_type_;
        }

        uint32_t get_stream_id(){
            return stream_id;
        }

        signal_flag get_signal_flag(){
            return flag;
        }

        const char* get_thing_id(){
            return thing_id;
        }

        bool has_data(){
            return data!=NULL;
        }

        bool has_resource(){
            return resource!=NULL;
        }

    public:
        void set_stream_id(uint32_t stream_id) {
            thinger_message::stream_id = stream_id;
        }

        void set_signal_flag(signal_flag const &flag) {
            thinger_message::flag = flag;
        }

        void set_thing_id(const char *thing_id) {
            size_t str_size = strlen(thing_id);
            this->thing_id = (char*)protoson::pool.allocate(str_size+1);
            memcpy(this->thing_id, thing_id, str_size+1);
        }

        void set_message_type(message_type type){
            message_type_ = type;
        }

    public:

        void operator=(const char* str){
            ((protoson::pson &) * this) = str;
        }

        operator protoson::pson&(){
            if(data==NULL){
                data = new (protoson::pool) protoson::pson;
                data_allocated = true;
            }
            return *data;
        }

        protoson::pson_array& resources(){
            return (protoson::pson_array&)get_resources();
        }

        protoson::pson& get_resources(){
            if(resource==NULL){
                resource = new (protoson::pool) protoson::pson;
            }
            return *resource;
        }

        protoson::pson& get_data(){
            return *this;
        }

        void set_data(protoson::pson& pson_data){
            if(data==NULL){
                data = &pson_data;
                data_allocated = false;
            }
        }

    };
}

#endif