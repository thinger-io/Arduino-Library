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

        bool connect(const char* username, const char* device_id, const char* credential)
        {
            // reset keep alive status for each connection
            keep_alive_response = true;

            thinger_message message;
            message.set_signal_flag(thinger_message::AUTH);
            message.resources().add(username).add(device_id).add(credential);
            return send_message_with_ack(message);
        }

    public:

        thinger_resource & operator[](const char* res){
            return resources_[res];
        }

        /**
         * Read a property stored in the server
         * @param property_identifier property identifier
         * @param data pson structure to be filled with the property data received from server
         * @return true if the property read was ok
         */
        bool get_property(const char* property_identifier, protoson::pson& data){
            thinger_message request;
            request.set_signal_flag(thinger_message::GET_PROPERTY);
            request.set_identifier(property_identifier);
            return send_message(request, data);
        }

        /**
         * Set a property in the server
         * @param property_identifier property identifier
         * @param data pson structure with the data to be stored in the server
         * @param confirm_write flag to control whether it is required a write confirmation or not
         * @return
         */
        bool set_property(const char* property_identifier, pson& data, bool confirm_write=false){
            thinger_message request;
            if(confirm_write) request.set_random_stream_id();
            request.set_signal_flag(thinger_message::SET_PROPERTY);
            request.set_identifier(property_identifier);
            request.set_data(data);
            return send_message_with_ack(request, confirm_write);
        }

        /**
         * Execute a resource in a remote device (without data)
         * @param device_name remote device identifier (must be connected to your account)
         * @param resource_name remote resource identifier (must be defined in the remote device)
         * @return
         */
        bool call_device(const char* device_name, const char* resource_name){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_DEVICE);
            message.set_identifier(device_name);
            message.resources().add(resource_name);
            return send_message(message);
        }

        /**
        * Execute a resource in a remote device (with a custom pson payload)
        * @param device_name remote device identifier (must be connected to your account)
        * @param resource_name remote resource identifier (must be defined in the remote device)
        * @param data pson structure to be sent to the remote resource input
        * @return
        */
        bool call_device(const char* device_name, const char* resource_name, pson& data){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_DEVICE);
            message.set_identifier(device_name);
            message.resources().add(resource_name);
            message.set_data(data);
            return send_message(message);
        }

        /**
        * Execute a resource in a remote device (with a payload from a local resource)
        * @param device_name remote device identifier (must be connected to your account)
        * @param resource_name remote resource identifier (must be defined in the remote device)
        * @param resource local device resource, as defined in code, i.e., thing["location"]
        * @return
        */
        bool call_device(const char* device_name, const char* resource_name, thinger_resource& resource){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_DEVICE);
            message.set_identifier(device_name);
            message.resources().add(resource_name);
            resource.fill_output(message.get_data());
            return send_message(message);
        }

        /**
         * Cal a server endpoint (without any data)
         * @param endpoint_name endpoint identifier, as defined in the server
         * @return
         */
        bool call_endpoint(const char* endpoint_name){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_ENDPOINT);
            message.set_identifier(endpoint_name);
            return send_message(message);
        }

        /**
         * Call a server endpoint
         * @param endpoint_name endpoint identifier, as defined in the server
         * @param data data in pson format to be used as data source for the endpoint call
         * @return
         */
        bool call_endpoint(const char* endpoint_name, pson& data){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_ENDPOINT);
            message.set_identifier(endpoint_name);
            message.set_data(data);
            return send_message(message);
        }

        /**
         * Call a server endpoint
         * @param endpoint_name endpoint identifier, as defined in the server
         * @param resource resource to be used as data source for the endpoint call, i.e., thing["location"]
         * @return
         */
        bool call_endpoint(const char* endpoint_name, thinger_resource& resource){
            thinger_message message;
            message.set_signal_flag(thinger_message::CALL_ENDPOINT);
            message.set_identifier(endpoint_name);
            resource.fill_output(message.get_data());
            return send_message(message);
        }

        /**
         * Call a server endpoint
         * @param endpoint_name endpoint identifier, as defined in the server
         * @param resource_name name of the resource to be used as data source for the endpoint call, i.e., "location"
         * @return
         */
        bool call_endpoint(const char* endpoint_name, const char* resource_name){
            return call_endpoint(endpoint_name, resources_[resource_name]);
        }

        /**
         * Write arbitrary data to a given bucket identifier
         * @param bucket_id bucket identifier
         * @param data data to write defined in a pson structure
         * @return
         */
        bool write_bucket(const char* bucket_id, pson& data){
            thinger_message message;
            message.set_signal_flag(thinger_message::BUCKET_DATA);
            message.set_identifier(bucket_id);
            message.set_data(data);
            return send_message(message);
        }

        /**
         * Write a resource to a given bucket identifier
         * @param bucket_id bucket identifier
         * @param resource_name resource defined in the code, i.e., thing["location"]
         * @return
         */
        bool write_bucket(const char* bucket_id, thinger_resource& resource){
            thinger_message message;
            message.set_signal_flag(thinger_message::BUCKET_DATA);
            message.set_identifier(bucket_id);
            resource.fill_output(message.get_data());
            return send_message(message);
        }

        /**
         * Write a resource to a given bucket identifier
         * @param bucket_id bucket identifier
         * @param resource_name resource identifier defined in the code, i.e, "location"
         * @return
         */
        bool write_bucket(const char* bucket_id, const char* resource_name){
            return write_bucket(bucket_id, resources_[resource_name]);
        }

        /**
         * Stream the given resource
         * @param resource resource defined in the code, i.e, thing["location"]
         * @param type STREAM_EVENT or STREAM_SAMPLE, depending if the stream was an event or a scheduled sampling
         */
        void stream_resource(thinger_resource& resource, thinger_message::signal_flag type){
            thinger_message message;
            message.set_stream_id(resource.get_stream_id());
            message.set_signal_flag(type);
            resource.fill_api_io(message.get_data());
            send_message(message);
        }

         /**
          * Stream the given resource. There should be any process listening for such resource, i.e., over a server websocket.
          * @param resource resource defined in the code, i.e, thing["location"]
          * @return true if there was some external process listening for this resource and the resource was transmitted
          */
        bool stream(thinger_resource& resource){
            if(resource.stream_enabled()){
                stream_resource(resource, thinger_message::STREAM_EVENT);
                return true;
            }
            return false;
        }

        /**
         * Stream the given resource. There should be any process listening for such resource, i.e., over a server websocket.
         * @param resource resource identifier defined in the code, i.e, "location"
         * @return true if there was some external process listening for this resource and the resource was transmitted
         */
        bool stream(const char* resource){
            return stream(resources_[resource]);
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
                thinger_message message;
                if(read_message(message)==MESSAGE) handle_request_received(message);
            }

            // handle keep alive (send keep alive to server to prevent disconnection)
            if(current_time-last_keep_alive>KEEP_ALIVE_MILLIS){
                if(keep_alive_response){
                    last_keep_alive = current_time;
                    keep_alive_response = false;
                    send_keep_alive();
                }else{
                    disconnected();
                }
            }

            // handle streaming resources
            if(thinger_resource::get_streaming_counter()>0){
                handle_streaming(resources_, current_time);
            }
        }

    private:

        /**
         * Iterates over all resources and subresources to determine when a streaming is required.
         * @param resources
         * @param current_time
         */
        void handle_streaming(thinger_map<thinger_resource>& resources, unsigned long current_time){
            thinger_map<thinger_resource>::entry* current = resources.begin();
            while(current!=NULL){
                thinger_resource& resource = current->value_;
                if(resource.stream_required(current_time)){
                    stream_resource(resource, thinger_message::STREAM_SAMPLE);
                }
                thinger_map<thinger_resource>& sub_resources = resource.get_resources();
                if(!sub_resources.empty()){
                    handle_streaming(sub_resources, current_time);
                }
                current = current->next_;
            }
        }

        /**
         * Decode a message from the current connection. It should be called when there are bytes available for reading.
         * @param message reference to the message that will be filled with the decoded information
         * @return true or false if the message passed in reference was filled with a valid message.
         */
        message_type read_message(thinger_message& message){
            uint32_t type = 0;
            if(decoder.pb_decode_varint32(type)){
                switch(type){
                    case MESSAGE: {
                        // decode message size & message itself
                        uint32_t size = 0;
                        return decoder.pb_decode_varint32(size) && decoder.decode(message, size) ? MESSAGE : NONE;
                    }
                    case KEEP_ALIVE: {
                        // update our keep_alive flag (connection active)
                        keep_alive_response = true;
                        // skip size bytes in keep alive (always 0)
                        return decoder.pb_skip_varint() ? KEEP_ALIVE : NONE;
                    }
                }
            }
            return NONE;
        }

        /**
         * Wait for a server response, and optionally store the response payload on the provided PSON structure
         * @param request source message that will be used forf
         * @param payload
         * @return true if the response was received and succeed (REQUEST_OK in signal flag)
         */
        bool wait_response(thinger_message& request, protoson::pson* payload = NULL){
            do{
                // try to read an incoming message
                thinger_message response;
                message_type type = read_message(response);
                switch(type){
                    // message received
                    case MESSAGE:
                        if(request.get_stream_id() == response.get_stream_id()){
                            // copy response payload to provided structure
                            if(payload != NULL && response.has_data()) pson::swap(response.get_data(), *payload);
                            return response.get_signal_flag()==thinger_message::REQUEST_OK;
                        }
                        handle_request_received(response);
                        break;
                        // keep alive is handled inside read_message automatically
                    case KEEP_ALIVE:
                        break;
                        // maybe a timeout while reading a message
                    case NONE:
                        return false;
                }
            }while(true);
        }

        /**
         * Write a message to the socket
         * @param message
         * @return true if success
         */
        bool write_message(thinger_message& message){
            thinger_encoder sink;
            sink.encode(message);
            encoder.pb_encode_varint(MESSAGE);
            encoder.pb_encode_varint(sink.bytes_written());
            encoder.encode(message);
            return write(NULL, 0, true);
        }

        /**
         * Send a message
         * @param message message to be sent
         * @return true if the message was written to the socket
         */
        bool send_message(thinger_message& message){
            return write_message(message);
        }

        /**
         * Send a message and optionally wait for server acknowledgement
         * @param message message to be sent
         * @param wait_ack true if ack is required
         * @return true if the message was acknowledged by the server.
         */
        bool send_message_with_ack(thinger_message& message, bool wait_ack=true){
            return write_message(message) && (!wait_ack || wait_response(message));
        }

        /**
         * Send a message and wait for server ack and response payload
         * @param message message to be sent
         * @param data protoson::pson structure to be filled with the response payload
         * @return true if the message was acknowledged by the server.
         */
        bool send_message(thinger_message& message, protoson::pson& data){
            return write_message(message) && wait_response(message, &data);
        }

        /**
         * Send a keep alive to the server
         * @return true if the keep alive was written to the socket
         */
        bool send_keep_alive(){
            encoder.pb_encode_varint(KEEP_ALIVE);
            encoder.pb_encode_varint(0);
            return write(NULL, 0, true);
        }

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