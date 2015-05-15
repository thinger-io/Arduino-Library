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

#ifndef THINGER_RESOURCE_HPP
#define THINGER_RESOURCE_HPP

#include "thinger_map.hpp"
#include "pson.h"
#include "thinger_message.hpp"

namespace thinger{


class thinger_resource {

public:
    enum io_type {
        none                = 0,
        run                 = 1,
        pson_in             = 2,
        pson_out            = 3,
        pson_in_pson_out    = 4
    };

    enum access_type{
        PRIVATE     = 0,
        PROTECTED   = 1,
        PUBLIC      = 2,
        NONE        = 3
    };

private:

    union callback{
        void (*run)();
        void (*pson_in)(protoson::pson& in);
        void (*pson_out)(protoson::pson& out);
        void (*pson_in_pson_out)(protoson::pson& in, protoson::pson& out);
    };

    io_type io_type_;
    access_type access_type_;
    callback callback_;
    thinger_map<thinger_resource> sub_resources_;

public:
    thinger_resource() : io_type_(none), access_type_(PRIVATE)
    {
    }

    thinger_resource * find(const char* res)
    {
        return sub_resources_.find(res);
    }

    thinger_resource & operator[](const char* res){
        return sub_resources_[res];
    }

    thinger_resource & operator()(access_type type){
        access_type_ = type;
        return *this;
    }

    io_type get_io_type(){
        return io_type_;
    }

    access_type get_access_type(){
        return access_type_;
    }

    void fill_api(protoson::pson_object& content){
        if(io_type_!=none){
            content["al"] = access_type_;
            content["fn"] = io_type_;
        }
        thinger_map<thinger_resource>::entry* current = sub_resources_.begin();
        if(current!=NULL){
            protoson::pson_object& actions = content["/"];
            do{
                current->value_.fill_api(actions[current->key_]);
                current = current->next_;
            }while(current!=NULL);
        }
    }

    void fill_api_io(protoson::pson_object& content){
        if(io_type_ == pson_in){
            callback_.pson_in(content["in"]);
        }else if(io_type_ == pson_out){
            callback_.pson_out(content["out"]);
        }else if(io_type_ == pson_in_pson_out){
            callback_.pson_in_pson_out(content["in"], content["out"]);
        }
    }

public:

    /**
     * Establish a function without input or output parameters
     */
    void operator=(void (*run_function)()){
        io_type_ = run;
        callback_.run = run_function;
    }

    /**
     * Establish a function with input parameters
     */
    void operator<<=(void (*in_function)(protoson::pson& in)){
        io_type_ = pson_in;
        callback_.pson_in = in_function;
    }

    /**
     * Establish a function that only generates an output
     */
    void operator>>=(void (*out_function)(protoson::pson& out)){
        io_type_ = pson_out;
        callback_.pson_out = out_function;
    }

    /**
     * Establish a function that can receive input parameters and generate an output
     */
    void operator=(void (*pson_in_pson_out_function)(protoson::pson& in, protoson::pson& out)){
        io_type_ = pson_in_pson_out;
        callback_.pson_in_pson_out = pson_in_pson_out_function;
    }

    void handle_request(thinger_message& request, thinger_message& response){
        switch (io_type_){
            case run:
                callback_.run();
                break;
            case pson_in:
                callback_.pson_in(request);
                break;
            case pson_out:
                callback_.pson_out(response);
                break;
            case pson_in_pson_out:
                callback_.pson_in_pson_out(request, response);
                break;
            case none:
                break;
        }
    }
};

}

#endif