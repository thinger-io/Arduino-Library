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

    using namespace protoson;

    class thinger : public thinger_io{
    public:
        thinger() :
                encoder(*this),
                decoder(*this),
                last_keep_alive(0),
                keep_alive_response(true)
        {
        }

        virtual ~thinger(){
        }

    private:
        thinger_write_encoder encoder;
        thinger_read_decoder decoder;
        unsigned long last_keep_alive;
        bool keep_alive_response;
        thinger_map<thinger_resource> resources_;

    protected:
        /**
         * Can be override to start reconnection process
         */
        virtual void disconnected(){
            // stop all streaming resources after disconnect
            if(thinger_resource::get_streaming_counter()>0) {
                thinger_map<thinger_resource>::entry* current = resources_.begin();
                while(current!=NULL){
                    current->value_.disable_streaming();
                    current = current->next_;
                }
            }
        }

        /**
         * Stream a given resource
         */
        void stream_resource(thinger_resource& resource, thinger_message::signal_flag type){
            thinger_message message;
            message.set_stream_id(resource.get_stream_id());
            message.set_signal_flag(type);
            resource.fill_api_io(message.get_data());
            send_message(message);
        }

    public:

        thinger_resource & operator[](const char* res){
            return resources_[res];
        }

        bool connect(const char* username, const char* device_id, const char* credential)
        {
            // reset keep alive status for each connection
            keep_alive_response = true;

            thinger_message message;
            message.set_signal_flag(thinger_message::AUTH);
            message.resources().add(username).add(device_id).add(credential);
            if(!send_message(message)) return false;

            thinger_message response;
            return read_message(response) && response.get_signal_flag() == thinger_message::REQUEST_OK;
        }

        bool call_device(const char* device_name, const char* resource_name){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_DEVICE);
            message.set_identifier(device_name);
            message.resources().add(resource_name);
            return send_message(message);
        }

        bool call_device(const char* device_name, const char* resource_name, pson& data){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_DEVICE);
            message.set_identifier(device_name);
            message.resources().add(resource_name);
            message.set_data(data);
            return send_message(message);
        }

        bool call_device(const char* device_name, const char* resource_name, thinger_resource& resource){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_DEVICE);
            message.set_identifier(device_name);
            message.resources().add(resource_name);
            resource.fill_output(message.get_data());
            return send_message(message);
        }

        bool call_endpoint(const char* endpoint_name){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_ENDPOINT);
            message.set_identifier(endpoint_name);
            return send_message(message);
        }

        bool call_endpoint(const char* endpoint_name, pson& data){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_ENDPOINT);
            message.set_identifier(endpoint_name);
            message.set_data(data);
            return send_message(message);
        }

        bool call_endpoint(const char* endpoint_name, thinger_resource& resource){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_ENDPOINT);
            message.set_identifier(endpoint_name);
            resource.fill_output(message.get_data());
            return send_message(message);
        }

        bool call_endpoint(const char* endpoint_name, const char* resource_name){
            return call_endpoint(endpoint_name, resources_[resource_name]);
        }

        /**
         * Stream the given resource. This resource should be previously requested by an external process.
         * Otherwise, the resource will not be streamed as nothing will be listening for it.
         */
        bool stream(thinger_resource& resource){
            if(resource.stream_enabled()){
                stream_resource(resource, thinger_message::STREAM_EVENT);
                return true;
            }
            return false;
        }

        bool stream(const char* resource){
            return stream(resources_[resource]);
        }

        bool send_message(thinger_message& message)
        {
            thinger_encoder sink;
            sink.encode(message);
            encoder.pb_encode_varint(MESSAGE);
            encoder.pb_encode_varint(sink.bytes_written());
            encoder.encode(message);
            return write(NULL, 0, true);
        }

        void handle(unsigned long current_time, bool bytes_available)
        {
            // handle input
            if(bytes_available){
                handle_input();
            }

            // handle keep alive
            if(current_time-last_keep_alive>KEEP_ALIVE_MILLIS){
                if(keep_alive_response){
                    last_keep_alive = current_time;
                    keep_alive_response = false;
                    encoder.pb_encode_varint(KEEP_ALIVE);
                    encoder.pb_encode_varint(0);
                    write(NULL, 0, true);
                }else{
                    disconnected();
                }
            }

            // handle streaming resources
            if(thinger_resource::get_streaming_counter()>0){
                thinger_map<thinger_resource>::entry* current = resources_.begin();
                while(current!=NULL){
                    if(current->value_.stream_required(current_time)){
                        stream_resource(current->value_, thinger_message::STREAM_SAMPLE);
                    }
                    current = current->next_;
                }
            }
        }

        bool read_message(thinger_message& message){
            uint32_t type = 0;
            if(decoder.pb_decode_varint32(type)){
                switch (type){
                    case MESSAGE: {
                        uint32_t size = 0;
                        return decoder.pb_decode_varint32(size) &&
                               decoder.decode(message, size);
                    }
                    case KEEP_ALIVE: {
                        keep_alive_response = true;
                        decoder.pb_skip_varint();
                    }
                }
            }
            return false;
        }

        bool handle_input(){
            thinger_message message;
            if(read_message(message)){
                handle_request_received(message);
                return true;
            }
            return false;
        }

    private:

        void handle_request_received(thinger_message& request)
        {
            thinger_message response(request);
            if(!request.has_resource()){
                response.set_signal_flag(thinger_message::REQUEST_ERROR);
            }
            else{
                thinger_resource * thing_resource = NULL;
                for(pson_array::iterator it = request.resources().begin(); it.valid(); it.next()){
                    if(!it.item().is_string()){
                        response.set_signal_flag(thinger_message::REQUEST_ERROR);
                        break;
                    }
                    const char* resource = it.item();
                    if(it.has_next()){
                        thing_resource = thing_resource == NULL ? resources_.find(resource) : thing_resource->find(resource);
                        if(thing_resource==NULL) {
                            response.set_signal_flag(thinger_message::REQUEST_ERROR);
                            break;
                        }
                    }else{
                        if(strcmp("api", resource)==0){
                            if(thing_resource==NULL){
                                thinger_map<thinger_resource>::entry* current = resources_.begin();
                                while(current!=NULL){
                                    current->value_.fill_api(response.get_data()[current->key_]);
                                    current = current->next_;
                                }
                            }else{
                                thing_resource->fill_api_io(response.get_data());
                            }
                        }else{
                            thing_resource = thing_resource == NULL ? resources_.find(resource) : thing_resource->find(resource);
                            if(thing_resource==NULL){
                                response.set_signal_flag(thinger_message::REQUEST_ERROR);
                            }else{
                                thing_resource->handle_request(request, response);
                                // stream enabled over a resource input -> notify the current state
                                if(thing_resource->stream_enabled() && thing_resource->get_io_type()==thinger_resource::pson_in){
                                    // send normal response
                                    send_message(response);
                                    // stream the event to notify the change
                                    return stream_resource(*thing_resource, thinger_message::STREAM_EVENT);
                                }
                            }
                        }
                    }
                }
            }
            // do not send responses to requests without a stream id as they will not reach any destination!
            if(response.get_stream_id()!=0){
                send_message(response);
            }
        }
    };
}

#endif