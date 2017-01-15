// The MIT License (MIT)
//
// Copyright (c) 2017 THINK BIG LABS S.L.
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

#ifndef THINGER_ENCODER_HPP
#define THINGER_ENCODER_HPP

#include "thinger_message.hpp"
#include "thinger_io.hpp"

namespace thinger{

    class thinger_encoder : public protoson::pson_encoder{

    protected:
        virtual bool write(const void *buffer, size_t size){
            return protoson::pson_encoder::write(buffer, size);
        }

    public:
        void encode(thinger_message& message){
            if(message.get_stream_id()!=0){
                pb_encode_varint(thinger_message::STREAM_ID, message.get_stream_id());
            }
            if(message.get_signal_flag()!=thinger_message::NONE){
                pb_encode_varint(thinger_message::SIGNAL_FLAG, message.get_signal_flag());
            }
            if(message.has_identifier()){
                pb_encode_tag(protoson::pson_type, thinger_message::IDENTIFIER);
                protoson::pson_encoder::encode(message.get_identifier());
            }
            if(message.has_resource()){
                pb_encode_tag(protoson::pson_type, thinger_message::RESOURCE);
                protoson::pson_encoder::encode(message.get_resources());
            }
            if(message.has_data()){
                pb_encode_tag(protoson::pson_type, thinger_message::PAYLOAD);
                protoson::pson_encoder::encode((protoson::pson&) message);
            }
        }
    };

    class thinger_write_encoder : public thinger_encoder{
    public:
        thinger_write_encoder(thinger_io& io) : io_(io)
        {}

    protected:
        virtual bool write(const void *buffer, size_t size){
            return io_.write((const char*)buffer, size) && protoson::pson_encoder::write(buffer, size);
        }

    private:
        thinger_io& io_;
    };

    class thinger_memory_encoder : public thinger_encoder{

    public:
        thinger_memory_encoder(uint8_t* buffer, size_t size) : buffer_(buffer), size_(size){}

    protected:
        virtual bool write(const void *buffer, size_t size){
            if(written_+size < size_){
                memcpy(buffer_ + written_, buffer, size);
                return protoson::pson_encoder::write(buffer, size);
            }
            return false;
        }

    private:
        uint8_t* buffer_;
        size_t size_;
    };

}

#endif