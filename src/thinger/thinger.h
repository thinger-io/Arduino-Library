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

        bool write_bucket(const char* bucket_id, pson& data){
            thinger_message message;
            message.set_signal_flag(thinger_message::BUCKET_DATA);
            message.set_identifier(bucket_id);
            message.set_data(data);
            return send_message(message);
        }

        bool write_bucket(const char* bucket_id, thinger_resource& resource){
            thinger_message message;
            message.set_signal_flag(thinger_message::BUCKET_DATA);
            message.set_identifier(bucket_id);
            resource.fill_output(message.get_data());
            return send_message(message);
        }

        bool write_bucket(const char* bucket_id, const char* resource_name){
            return write_bucket(bucket_id, resources_[resource_name]);
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
            /* TODO test this properly. Some devices (like AT based ones)
             * may fail to write, but are not necessarily disconnected */
            //bool result = write(NULL, 0, true);
            //if(!result) disconnected();
            //return result;
        }

        /**
         * This method should be called periodically, indicating the current timestamp, and if there are bytes
         * available in the connection
         * @param current_time in milliseconds, i.e., unix epoch or millis from start.
         * @param bytes_available true or false indicating if there is input data available for reading.
         */
        void handle(unsigned long current_time, bool bytes_available)
        {
            // handle input
            if(bytes_available){
                handle_input();
            }

            // handle keep alive (send keep alive to server to prevent disconnection)
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

        /**
         * Decode a message from the current connection. It should be called when there are bytes available for reading.
         * @param message reference to the message that will be filled with the decoded information
         * @return true or false if the message passed in reference was filled with a valid message.
         */
        bool read_message(thinger_message& message){
            uint32_t type = 0;
            if(decoder.pb_decode_varint32(type)){
                switch (type){
                    case MESSAGE: {
                        // decode message size & message itself
                        uint32_t size = 0;
                        return decoder.pb_decode_varint32(size) &&
                               decoder.decode(message, size);
                    }
                    case KEEP_ALIVE: {
                        // update our keep_alive flag (connection active)
                        keep_alive_response = true;
                        // skip size bytes in keep alive (always 0)
                        decoder.pb_skip_varint();
                    }
                }
            }
            return false;
        }

        /**
         * Handle any connection input. This method should be called when there is information in the input buffer
         * @return true or false if a message was decoded
         */
        bool handle_input(){
            thinger_message message;
            if(read_message(message)){
                // keep alive message is not decoded as a message
                handle_request_received(message);
                return true;
            }
            return false;
        }

    private:

        /**
         * Handle an incoming request from the server
         * @param request the message sent by the server
         */
        void handle_request_received(thinger_message& request)
        {
            // create a response message to any incoming request
            thinger_message response(request);

            // if there is no resource in the message, they are not asking for anything in our device
            if(!request.has_resource()){
                response.set_signal_flag(thinger_message::REQUEST_ERROR);
            }

            /*
             * decode the requested resource. A resource is an array of string identifiers, as resources may be
             * concatenated, i.e., temperature/degrees; tire1/pressure.
             */
            else{
                // pointer to the requested resource (not initialized by default)
                thinger_resource * thing_resource = NULL;

                for(pson_array::iterator it = request.resources().begin(); it.valid(); it.next()){

                    // if the resource name is not a string.. stop!
                    if(!it.item().is_string()){
                        response.set_signal_flag(thinger_message::REQUEST_ERROR);
                        break;
                    }

                    // get current resource, and check if there are more remaining resources
                    const char* resource = it.item();

                    // there are more sub resources in the array
                    if(it.has_next()){

                        // search the requested resource in the root, or just in the current resource (kept in thing_resource)
                        thing_resource = thing_resource == NULL ? resources_.find(resource) : thing_resource->find(resource);

                        // the requested resource is not available in the device or the resource... stop!
                        if(thing_resource==NULL) {
                            response.set_signal_flag(thinger_message::REQUEST_ERROR);
                            break;
                        }

                    // the current item is the latest resource name
                    }else{

                        // check if resource name is the special word "api" to fill the current resource state
                        if(strcmp("api", resource)==0){
                            // just fill the api over the device root
                            if(thing_resource==NULL){
                                thinger_map<thinger_resource>::entry* current = resources_.begin();
                                while(current!=NULL){
                                    current->value_.fill_api(response.get_data()[current->key_]);
                                    current = current->next_;
                                }
                            // fll the api over the specified resource
                            }else{
                                thing_resource->fill_api_io(response.get_data());
                            }

                        // just want to interact with the resource itself...
                        }else{
                            thing_resource = thing_resource == NULL ? resources_.find(resource) : thing_resource->find(resource);
                            // the resource is not available.. stop!
                            if(thing_resource==NULL){
                                response.set_signal_flag(thinger_message::REQUEST_ERROR);

                            // the resource is available, so, handle its i/o.
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