// The MIT License (MIT)
//
// Copyright (c) 2016 THINGER LTD
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

    enum message_type{
        MESSAGE             = 1,
        KEEP_ALIVE          = 2
    };

    class thinger_message{

    public:

        // fields for a thinger message (encoded as in protocol buffers)
        enum fields{
            STREAM_ID       = 1,
            SIGNAL_FLAG     = 2,
            UNUSED          = 3,
            RESOURCE        = 4,
            UNUSED2         = 5,
            PSON_PAYLOAD    = 6
        };

        // flags for describing a thinger message
        enum signal_flag {
            // GENERAL USED FLAGS
            REQUEST_OK          = 1,    // the request with the given stream id was successful
            REQUEST_ERROR       = 2,    // the request with the given stream id failed

            // SENT BY THE SERVER
            NONE                = 0,    // default resource action: just execute the given resource
            START_STREAM        = 3,    // enable a streaming resource (with stream_id, and resource filled, sample interval (in payload) is optional)
            STOP_STREAM         = 4,    // stop the streaming resource (with stream_id, and resource filled)

            // SENT BY DEVICE
            AUTH                = 5,
            STREAM_EVENT        = 6,    // means that the message data is related to a stream event
            STREAM_SAMPLE       = 7,    // means that the message is related to a periodical streaming sample
            CALL_ENDPOINT       = 8     // call the endpoint with the provided name (endpoint in resource, value passed in payload)
        };

    public:

        /**
         * Initialize a default response  message setting the same stream id of the source message,
         * and initializing the signal flag to ok. All remaining data or fields are empty
         */
        thinger_message(thinger_message& other) :
            stream_id(other.stream_id),
            flag(REQUEST_OK),
            resource(NULL),
            data(NULL),
            data_allocated(false)
        {}

        /**
         * Initialize a default empty message
         */
        thinger_message() :
                stream_id(0),
                flag(NONE),
                resource(NULL),
                data(NULL),
                data_allocated(false)
        {}

        ~thinger_message(){
            // deallocate resource
            destroy(resource, protoson::pool);
            // deallocate paylaod if was allocated here
            if(data_allocated){
                destroy(data, protoson::pool);
            }
        }

    private:
        /// used for identifying a unique stream
        uint16_t stream_id;
        /// used for setting a stream signal
        signal_flag flag;
        /// used to identify a device resource
        protoson::pson* resource;
        /// used to fill a data payload in the message
        protoson::pson* data;
        /// flag to determine when the payload has been reserved
        bool data_allocated;

    public:

        uint16_t get_stream_id(){
            return stream_id;
        }

        signal_flag get_signal_flag(){
            return flag;
        }

        bool has_data(){
            return data!=NULL;
        }

        bool has_resource(){
            return resource!=NULL;
        }

    public:
        void set_stream_id(uint16_t stream_id) {
            thinger_message::stream_id = stream_id;
        }

        void set_signal_flag(signal_flag const &flag) {
            thinger_message::flag = flag;
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