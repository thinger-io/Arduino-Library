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

#ifndef THINGER_MESSAGE_HPP
#define THINGER_MESSAGE_HPP

#include "pson.h"

namespace thinger{

    enum message_type{
        NONE                = 0,
        MESSAGE             = 1,
        KEEP_ALIVE          = 2
    };

    class thinger_message{

    public:

        // fields for a thinger message (encoded as in protocol buffers)
        enum fields{
            STREAM_ID       = 1,
            SIGNAL_FLAG     = 2,
            IDENTIFIER      = 3,
            RESOURCE        = 4,
            UNUSED1         = 5,
            PAYLOAD         = 6
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
            CALL_ENDPOINT       = 8,    // call the endpoint with the provided name (endpoint_id in identifier, value passed in payload)
            CALL_DEVICE         = 9,    // call a given device (device_id in identifier, resource in resource, and value, passed in payload)
            BUCKET_DATA         = 10,    // call the bucket with the provided name (bucket_id in identifier, value passed in payload)
            GET_PROPERTY        = 11,    // call the bucket with the provided name (bucket_id in identifier, value passed in payload)
            SET_PROPERTY        = 12    // call the bucket with the provided name (bucket_id in identifier, value passed in payload)
        };

    public:

        /**
         * Initialize a default response  message setting the same stream id of the source message,
         * and initializing the signal flag to ok. All remaining data or fields are empty
         */
        thinger_message(thinger_message& other) :
            stream_id(other.stream_id),
            flag(REQUEST_OK),
            identifier(NULL),
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
            identifier(NULL),
            resource(NULL),
            data(NULL),
            data_allocated(false)
        {}

        ~thinger_message(){
            // deallocate identifier
            protoson::pool.destroy(identifier);
            // deallocate resource
            protoson::pool.destroy(resource);
            // deallocate paylaod if was allocated here
            if(data_allocated){
                protoson::pool.destroy(data);
            }
        }

    private:
        /// used for identifying a unique stream
        uint16_t stream_id;
        /// used for setting a stream signal
        signal_flag flag;
        /// used to identify a device, an endpoint, or a bucket
        protoson::pson* identifier;
        /// used to identify an specific resource over the identifier
        protoson::pson* resource;
        /// used to send a data payload in the message
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

        bool has_identifier(){
            return identifier!=NULL;
        }

        bool has_resource(){
            return resource!=NULL;
        }

    public:
        void set_stream_id(uint16_t stream_id) {
            thinger_message::stream_id = stream_id;
        }

        void set_random_stream_id(){
            // TODO, random seed
            thinger_message::stream_id = rand();
        }

        void set_signal_flag(signal_flag const &flag) {
            thinger_message::flag = flag;
        }

        void set_identifier(const char* id){
            if(identifier==NULL){
                identifier = protoson::pool.allocate<protoson::pson>();
            }
            (*identifier) = id;
        }

        void clean_identifier(){
            protoson::pool.destroy(identifier);
            identifier = NULL;
        }

        void clean_resource(){
            protoson::pool.destroy(resource);
            resource = NULL;
        }

        void clean_data(){
            if(data_allocated){
                protoson::pool.destroy(data);
            }
            data = NULL;
        }

    public:

        void operator=(const char* str){
            ((protoson::pson &) * this) = str;
        }

        operator protoson::pson&(){
            if(data==NULL){
                data = protoson::pool.allocate<protoson::pson>();
                data_allocated = true;
            }
            return *data;
        }

        protoson::pson_array& resources(){
            return (protoson::pson_array&)get_resources();
        }

        protoson::pson& get_resources(){
            if(resource==NULL){
                resource = protoson::pool.allocate<protoson::pson>();
            }
            return *resource;
        }

        protoson::pson& get_identifier(){
            if(identifier==NULL){
                identifier = protoson::pool.allocate<protoson::pson>();
            }
            return *identifier;
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