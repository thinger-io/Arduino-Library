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

#ifndef THINGER_RESOURCE_HPP
#define THINGER_RESOURCE_HPP

#include "thinger_map.hpp"
#include "pson.h"
#include "thinger_message.hpp"

#ifdef __has_include
#  if __has_include(<functional>)
#    include <functional>
#    define THINGER_USE_FUNCTIONAL
#  endif
#endif

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
        INTERNAL    = 3
    };

    static unsigned int& get_streaming_counter(){
        // used to know the total number of streams
        static unsigned int streaming_count_ = 0;
        return streaming_count_;
    }

private:

    // calback for function, input, output, or input/output
#ifdef THINGER_USE_FUNCTIONAL

    struct callback{
        std::function<void()> run;
        std::function<void(protoson::pson& io)> pson;
        std::function<void(protoson::pson& in, protoson::pson& out)> pson_in_pson_out;
    };

#else

    union callback{
        void (*run)();
        void (*pson)(protoson::pson& io);
        void (*pson_in_pson_out)(protoson::pson& in, protoson::pson& out);
    };

#endif

    // used for defining the resource
    io_type io_type_;
    access_type access_type_;
    callback callback_;

    // used for allowing resource streaming (both periodically or by events)
    uint16_t stream_id_;

    // used for periodic stream events
    unsigned long streaming_freq_;
    unsigned long last_streaming_;

    // TODO change to pointer so it is not using more than a pointer size if not used?
    thinger_map<thinger_resource> sub_resources_;

    void enable_streaming(uint16_t stream_id, unsigned long streaming_freq){
        stream_id_ = stream_id;
        if(streaming_freq_==0 && streaming_freq>0){
            get_streaming_counter()++;
        }else if(streaming_freq_>0 && streaming_freq==0){
            get_streaming_counter()--;
        }
        streaming_freq_ = streaming_freq;
        last_streaming_ = 0;
    }

public:
    thinger_resource() : io_type_(none), access_type_(PRIVATE), stream_id_(0), streaming_freq_(0), last_streaming_(0)
    {}

    void disable_streaming(){
        stream_id_ = 0;
        if(streaming_freq_>0){
            get_streaming_counter()--;
        }
        streaming_freq_ = 0;
    }

    bool stream_enabled(){
        return stream_id_ > 0;
    }

    uint32_t get_stream_id(){
        return stream_id_;
    }

    bool stream_required(unsigned long timestamp){
        // sample interval is activated
        if(streaming_freq_>0){
            if(timestamp-last_streaming_>=streaming_freq_){
                last_streaming_ = timestamp;
                return true;
            }
        }
        return false;
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
            callback_.pson(content["in"]);
        }else if(io_type_ == pson_out){
            callback_.pson(content["out"]);
        }else if(io_type_ == pson_in_pson_out){
            callback_.pson_in_pson_out(content["in"], content["out"]);
        }
    }

    void fill_output(protoson::pson& content){
        if(io_type_ == pson_out){
            callback_.pson(content);
        }
    }

    thinger_map<thinger_resource>& get_resources(){
        return sub_resources_;
    }

public:

#ifdef THINGER_USE_FUNCTIONAL

    /**
     * Establish a function without input or output parameters
     */
    void operator=(std::function<void()> run_function){
        io_type_ = run;
        callback_.run = run_function;
    }

    /**
     * Establish a function without input or output parameters
     */
    void set_function(std::function<void()> run_function){
        io_type_ = run;
        callback_.run = run_function;
    }

    /**
     * Establish a function with input parameters
     */
    void operator<<(std::function<void(protoson::pson&)> in_function){
        io_type_ = pson_in;
        callback_.pson = in_function;
    }

    /**
     * Establish a function with input parameters
     */
    void set_input(std::function<void(protoson::pson&)> in_function){
        io_type_ = pson_in;
        callback_.pson = in_function;
    }

    /**
     * Establish a function that only generates an output
     */
    void operator>>(std::function<void(protoson::pson&)> out_function){
        io_type_ = pson_out;
        callback_.pson = out_function;
    }

    /**
     * Establish a function that only generates an output
     */
    void set_output(std::function<void(protoson::pson&)> out_function){
        io_type_ = pson_out;
        callback_.pson = out_function;
    }

    /**
     * Establish a function that can receive input parameters and generate an output
     */
    void operator=(std::function<void(protoson::pson& in, protoson::pson& out)> pson_in_pson_out_function){
        io_type_ = pson_in_pson_out;
        callback_.pson_in_pson_out = pson_in_pson_out_function;
    }

    /**
     * Establish a function that can receive input parameters and generate an output
     */
    void set_input_output(std::function<void(protoson::pson& in, protoson::pson& out)> pson_in_pson_out_function){
        io_type_ = pson_in_pson_out;
        callback_.pson_in_pson_out = pson_in_pson_out_function;
    }

#else

    /**
     * Establish a function without input or output parameters
     */
    void operator=(void (*run_function)()){
        io_type_ = run;
        callback_.run = run_function;
    }

    /**
     * Establish a function without input or output parameters
     */
    void set_function(void (*run_function)()){
        io_type_ = run;
        callback_.run = run_function;
    }

    /**
     * Establish a function with input parameters
     */
    void operator<<(void (*in_function)(protoson::pson& in)){
        io_type_ = pson_in;
        callback_.pson = in_function;
    }

    /**
     * Establish a function with input parameters
     */
    void set_input(void (*in_function)(protoson::pson& in)){
        io_type_ = pson_in;
        callback_.pson = in_function;
    }

    /**
     * Establish a function that only generates an output
     */
    void operator>>(void (*out_function)(protoson::pson& out)){
        io_type_ = pson_out;
        callback_.pson = out_function;
    }

    /**
     * Establish a function that only generates an output
     */
    void set_output(void (*out_function)(protoson::pson& out)){
        io_type_ = pson_out;
        callback_.pson = out_function;
    }

    /**
     * Establish a function that can receive input parameters and generate an output
     */
    void operator=(void (*pson_in_pson_out_function)(protoson::pson& in, protoson::pson& out)){
        io_type_ = pson_in_pson_out;
        callback_.pson_in_pson_out = pson_in_pson_out_function;
    }

    /**
     * Establish a function that can receive input parameters and generate an output
     */
    void set_input_output(void (*pson_in_pson_out_function)(protoson::pson& in, protoson::pson& out)){
        io_type_ = pson_in_pson_out;
        callback_.pson_in_pson_out = pson_in_pson_out_function;
    }

#endif

    /**
     * Handle a request and fill a possible response
     */
    void handle_request(thinger_message& request, thinger_message& response){
        switch(request.get_signal_flag()){
            // default action over the stream (run the resource)
            case thinger_message::NONE:
                switch (io_type_){
                    case pson_in:
                        callback_.pson(request);
                        break;
                    case pson_out:
                        callback_.pson(response);
                        break;
                    case run:
                        callback_.run();
                        break;
                    case pson_in_pson_out:
                        callback_.pson_in_pson_out(request, response);
                        break;
                    case none:
                        break;
                }
                break;
            // flag for starting a resource stream
            case thinger_message::START_STREAM:
                enable_streaming(request.get_stream_id(), request.get_data());
                break;
            // flat for stopping a resource stream
            case thinger_message::STOP_STREAM:
                disable_streaming();
                break;
            default:
                break;
        }
    }
};

}

#endif