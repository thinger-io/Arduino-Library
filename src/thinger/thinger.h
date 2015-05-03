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

#ifndef THINGER_H
#define THINGER_H

#include "pson.h"
#include "thinger_map.hpp"
#include "thinger_resource.hpp"
#include "thinger_message.hpp"
#include "thinger_encoder.hpp"
#include "thinger_decoder.hpp"
#include "thinger_message.hpp"
#include "thinger_io.hpp"

#define KEEP_ALIVE_MILLIS 60000

namespace thinger{

    class thinger : public thinger_io{
    public:
        thinger() :
                encoder(*this),
                decoder(*this),
                last_keep_alive(0)
        {
        }

        virtual ~thinger(){
        }

    private:
        thinger_write_encoder encoder;
        thinger_read_decoder decoder;
        unsigned long last_keep_alive;
        thinger_map<thinger_resource> resources_;

    public:

        thinger_resource & operator[](const char* res){
            return resources_[res];
        }

        bool connect(const char* username, const char* device_id, const char* credential)
        {
            thinger_message message;
            message.resources().add("login").add(username).add(device_id).add(credential);
            if(!send_message(message)) return false;

            thinger_message response;
            return read_message(response) && response.get_signal_flag() == thinger_message::REQUEST_OK;
        }

        bool call_endpoint(const char* endpoint_name){
            thinger_message message;
            message.resources().add("ep").add(endpoint_name);
            if(!send_message(message)) return false;
            return true;
        }

        bool call_endpoint(const char* endpoint_name, protoson::pson& data){
            thinger_message message;
            message.set_data(data);
            message.resources().add("ep").add(endpoint_name);
            if(!send_message(message)) return false;
            return true;
        }

        bool send_message(thinger_message& message)
        {
            thinger_encoder sink;
            sink.encode(message);
            encoder.pb_encode_varint(message.get_message_type());
            encoder.pb_encode_varint(sink.bytes_written());
            encoder.encode(message);
            write(NULL, 0);
            return true;
        }

        void handle(unsigned long current_time, bool bytes_available)
        {
            if(bytes_available){
                handle_input();
            }

            // send keep alive if required
            if(current_time-last_keep_alive > KEEP_ALIVE_MILLIS) {
                last_keep_alive = current_time;
                encoder.pb_encode_varint(thinger_message::KEEP_ALIVE);
                encoder.pb_encode_varint(0);
                write(NULL, 0);
            }
        }

        bool read_message(thinger_message& message){
            uint8_t message_type = decoder.pb_decode_varint32();
            switch (message_type){
                case thinger_message::MESSAGE: {
                    size_t size = decoder.pb_decode_varint32();
                    decoder.decode(message, size);
                }
                    break;
                case thinger_message::KEEP_ALIVE: {
                    size_t size = decoder.pb_decode_varint32();
                }
                    return false;
                default:
                    return false;
            }
            return true;
        }

        bool handle_input(){
            thinger_message message;
            if(read_message(message)){
                handle_request_received(message);
            }
            return true;
        }

    private:

        bool is_str(protoson::pson_array& array, const char* str, size_t index=0){
            size_t current_index = 0;
            for(protoson::pson_array::iterator it = array.begin(); it.valid() && current_index<=index; it.next()){
                if(current_index==index) return it.item().is_string() && strcmp(str, (const char*)it.item())==0;
                current_index++;
            }
            return false;
        }

        void handle_request_received(thinger_message& request)
        {
            if(request.has_resource() && is_str(request.resources(), "api")){
                thinger_message response(request);
                protoson::pson_object& content = response.get_data();
                thinger_map<thinger_resource>::entry* current = resources_.begin();
                while(current!=NULL){
                    current->value_.fill_api(content[current->key_]);
                    current = current->next_;
                }
                send_message(response);
                return;
            }

            // TODO integrate streaming resources for websockets and server sent events

            // TODO optimize this section. Code looks so tricky
            if(request.has_resource()){
                thinger_message response(request);
                protoson::pson_array::iterator it = request.resources().begin();
                thinger_resource * resource_action = NULL;
                while(it.valid() && it.item().is_string()){
                    thinger_resource * prev = resource_action;
                    // first iteration search on root
                    if(resource_action==NULL){
                        resource_action = resources_.find(it.item());
                        // search on sub resource
                    }else{
                        resource_action = resource_action->find(it.item());
                    }
                    // at this step something returned should return NULL, so stop searching
                    if(resource_action==NULL && prev!=NULL){
                        if(strcmp("api", it.item())==0){
                            prev->fill_api_io(response.get_data());
                            send_message(response);
                            return;
                        }
                        break;
                    }
                    if(resource_action==NULL){
                        break;
                    }
                    it.next();
                }
                if(resource_action!=NULL){
                    resource_action->handle_request(request, response);
                }else{
                    response.set_signal_flag(thinger_message::REQUEST_ERROR);
                }
                send_message(response);
            }
        }
    };
}

#endif