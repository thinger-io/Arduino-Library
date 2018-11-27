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

#ifndef PSON_HPP
#define PSON_HPP

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifndef ARDUINO
#include <string>
#endif

#ifndef UINT32_MAX
#define UINT32_MAX  4294967295U
#endif

/*
 * Dummy placement new operator to support old Arduino compilers where this operator is not defined
 * (and cannot be used from inside a class), and also to not overwrite global operator from modern
 * toolchains like Espressif SDKs
 */
inline void * operator new(size_t sz, void * here, void* dummy) { return here; }

namespace protoson {

    class memory_allocator{
    public:

        // allocate
        virtual void *allocate(size_t size) = 0;
        virtual void deallocate(void *) = 0;

        template <class T>
        T* allocate(){
            /*
             * placement new has UB if memory is NULL, so check there is memory
             * before trying to call the operator
             */
            if(void * memory = allocate(sizeof(T))){
                return new (memory, NULL) T();
            }
            return NULL;
        }

        template <class T>
        void destroy(T* p){
            if(p){
                p->~T();
                deallocate(p);
            }
        }
    };

    template<size_t buffer_size>
    class circular_memory_allocator : public memory_allocator{
    private:
        uint8_t buffer_[buffer_size];
        size_t index_;
    public:
        circular_memory_allocator() : index_(0) {
        }

        virtual void *allocate(size_t size) {
            if(size>buffer_size){
                return NULL;
            }
            if (index_ + size > buffer_size) {
                index_ = 0;
            }
            void *position = &buffer_[index_];
            index_ += size;
            return position;
        }

        virtual void deallocate(void *) {}
    };

    class dynamic_memory_allocator : public memory_allocator{
    public:
        virtual void *allocate(size_t size) {
            return malloc(size);
        }

        virtual void deallocate(void *ptr) {
            free(ptr);
        }
    };

    extern memory_allocator& pool;
}

namespace protoson {

    enum pb_wire_type{
        varint = 0,
        fixed_64 = 1,
        length_delimited = 2,
        fixed_32 = 5,
        pson_type = 6
    };

    template<class T>
    class pson_container {

        class list_item{
        public:
            list_item() : next_(NULL), previous_(NULL) {}
            ~list_item(){}

            T item_;
            list_item* next_;
            list_item* previous_;
        };

    public:

        class iterator{
        public:
            iterator(list_item *item) : current_(item) {
            }

        private:
            list_item * current_;

        public:

            bool next(){
                if(current_==NULL) return false;
                current_ = current_->next_;
                return true;
            }

            bool has_next(){
                return current_!=NULL && current_->next_!=NULL;
            }

            bool valid(){
                return current_!=NULL;
            }

            T& item(){
                return current_->item_;
            }
        };

    private:
        list_item* item_;
        list_item* last_;

    public:
        iterator begin() const{
            return iterator(item_);
        }

        iterator end() const{
            return iterator(last_);
        }

        pson_container() : item_(NULL), last_(NULL) {
        }

        ~pson_container(){
            clear();
        }

        size_t size() const{
            size_t size = 0;
            list_item* current = item_;
            while(current!=NULL){
                current = current->next_;
                size++;
            }
            return size;
        }

        T* operator[](size_t index){
            list_item* current = item_;
            size_t current_index = 0;
            while(current!=NULL){
                if(current_index==index){
                    return &current->item_;
                }
                current = current->next_;
                current_index++;
            }
            return NULL;
        }

        void clear(){
            while(last_!=NULL){
                list_item* previous = last_->previous_;
                pool.destroy(last_);
                last_ = previous;
            }
        }

        T* create_item(){
            list_item* new_list_item = pool.allocate<list_item>();
            if(new_list_item==NULL) return NULL;
            if(item_==NULL){
                item_ = new_list_item;
            }else{
                last_->next_ = new_list_item;
                new_list_item->previous_ = last_;
            }
            last_ = new_list_item;
            return &(new_list_item->item_);
        }
    };

    class pson_object;
    class pson_array;

    class pson {
    public:
        enum field_type {
            null_field      = 0,
            varint_field    = 1,
            svarint_field   = 2,
            float_field     = 3,
            double_field    = 4,
            true_field      = 5,
            false_field     = 6,
            zero_field      = 7,
            one_field       = 8,
            string_field    = 9,
            empty_string    = 10,
            bytes_field     = 11,
            empty_bytes     = 12,
            object_field    = 13,
            array_field     = 14,
            empty           = 15
            // a message tag is encoded in a 128-base varint [1-bit][3-bit wire type][4-bit field]
            // we have up to 4 bits (0-15) for encoding fields in the first byte
        };

        // interchange two different containers
        static void swap(pson& source, pson& destination){
            // destroy destination container data (if any)
            destination.~pson();
            // override fields
            destination.value_ = source.value_;
            destination.field_type_ = source.field_type_;
            // 'clear' source container
            source.value_ = NULL;
            source.field_type_ = empty;
        }

        bool is_boolean() const{
            return field_type_ == true_field || field_type_ == false_field;
        }

        bool is_string() const{
            return field_type_ == string_field;
        }

        bool is_bytes() const{
            return field_type_ == bytes_field;
        }

        bool is_number() const{
            return  field_type_ == varint_field     ||
                    field_type_ == svarint_field    ||
                    field_type_ == float_field      ||
                    field_type_ == double_field     ||
                    field_type_ == zero_field       ||
                    field_type_ == one_field;
        }

        bool is_float() const{
            return  field_type_ == float_field      ||
                    field_type_ == double_field;
        }

        bool is_integer() const{
            return  field_type_ == varint_field     ||
                    field_type_ == svarint_field    ||
                    field_type_ == zero_field       ||
                    field_type_ == one_field;
        }

        bool is_object() const{
            return field_type_ == object_field;
        }

        bool is_array() const{
            return field_type_ == array_field;
        }

        bool is_null() const{
            return field_type_ == null_field;
        }

        bool is_empty() const{
            return field_type_ == empty;
        }

        pson() : value_(NULL), field_type_(empty) {
        }

        template<class T>
        pson(T value) : value_(NULL), field_type_(empty){
            *this = value;
        }

        ~pson(){
            if(field_type_==object_field){
                pool.destroy((pson_object *) value_);
            }else if(field_type_==array_field) {
                pool.destroy((pson_array *) value_);
            }else{
                pool.deallocate(value_);
            }
            value_ = NULL;
            field_type_ = empty;
        }

        template<class T>
        void operator=(T value)
        {
            if(value==0){
                field_type_ = zero_field;
            }else if(value==1) {
                field_type_ = one_field;
            }else{
                uint64_t uint_value = value>0 ? value : -value;
                if(allocate(pson::get_varint_size(uint_value))){
                    pb_encode_varint(uint_value);
                    field_type_ = value>0 ? varint_field : svarint_field;
                }
            }
        }

        void operator=(bool value){
            field_type_ = value ? true_field : false_field;
        }

        void operator=(float value) {
            if(value==(int32_t)value){
                *this = (int32_t) value;
            }else{
                field_type_ = float_field;
                set(value);
            }
        }

        void operator=(double value) {
            if(value==(int64_t)value) {
                *this = (int64_t) value;
            }else if(fabs(value-(float)value)<=0.00001){
                field_type_ = float_field;
                set((float)value);
            }else{
                field_type_ = double_field;
                set(value);
            }
        }

        void operator=(const char *str) {
            size_t str_size = strlen(str);
            if(str_size==0){
                field_type_ = empty_string;
            }else if(allocate(str_size+1)){
                memcpy(value_, str, str_size+1);
                field_type_ = string_field;
            }
        }

        void set_bytes(void* bytes, size_t size) {
            if(size>0){
                size_t varint_size = get_varint_size(size);
                if(allocate(varint_size+size)){
                    pb_encode_varint(size);
                    memcpy(((uint8_t*)value_)+varint_size, bytes, size);
                    field_type_ = bytes_field;
                }
            }else{
                field_type_ = empty_bytes;
            }
        }

        bool get_bytes(const void*& bytes, size_t& size){
            switch(field_type_){
                case bytes_field:
                    size = pb_decode_varint();
                    bytes = (uint8_t*) value_ + get_varint_size(size);
                    return true;
                case empty:
                    field_type_ = empty_bytes;
                default:
                    return false;
            }
        }

        bool allocate(size_t size){
            if(value_ == NULL){
                value_ = pool.allocate(size);
                return value_!=NULL;
            }
            return false;
        }

        template <class T>
        bool allocate(){
            if(value_ == NULL){
                value_ = pool.allocate<T>();
                return value_!=NULL;
            }
            return false;
        }

        operator pson_object &();
        operator pson_array &();
        pson & operator[](const char *name);

        operator const char *() {
            switch(field_type_){
                case string_field:
                    return (const char*) value_;
                case empty:
                    field_type_ = empty_string;
                default:
                    return "";
            }
        }

        operator bool(){
            switch(field_type_){
                case zero_field:
                case false_field:
                    return false;
                case one_field:
                case true_field:
                    return true;
                case empty:
                    field_type_ = false_field;
                default:
                    return 0;
            }
        }

        operator char(){
            return get_value<char>();
        }

        operator unsigned char(){
            return get_value<unsigned char>();
        }

        operator short(){
            return get_value<short>();
        }

        operator unsigned short(){
            return get_value<unsigned short>();
        }

        operator int(){
            return get_value<int>();
        }

        operator unsigned int(){
            return get_value<unsigned int>();
        }

        operator long(){
            return get_value<long>();
        }

        operator unsigned long(){
            return get_value<unsigned long>();
        }

        operator float(){
            return get_value<float>();
        }

        operator double(){
            return get_value<double>();
        }

        template<class T>
        operator T() {
            return get_value<T>();
        }

        template<class T>
        T get_value(){
            switch(field_type_){
                case zero_field:
                case false_field:
                    return 0;
                case one_field:
                case true_field:
                    return 1;
                case float_field:
                    return *(float*)value_;
                case double_field:
                    return *(double*)value_;
                case varint_field:
                    return pb_decode_varint();
                case svarint_field:
                    return -pb_decode_varint();
                case empty:
                    field_type_ = zero_field;
                default:
                    return 0;
            }
        }

        void* get_value(){
            return value_;
        }

        field_type get_type() const{
            return field_type_;
        }

        void set_null(){
            field_type_ = null_field;
            // TODO free existing value_ (if any)
        }

        void set_type(field_type type){
            field_type_ = type;
        }

        uint8_t get_varint_size(uint64_t value) const{
            uint8_t size = 1;
            while(value>>=7) size++;
            return size;
        }

        void pb_encode_varint(uint64_t value) const
        {
            uint8_t count = 0;
            do
            {
                uint8_t byte = (uint8_t)(value & 0x7F);
                value >>= 7;
                if(value) byte |= 0x80;
                ((uint8_t*)value_)[count] = byte;
                count++;
            }while(value);
        }

        uint64_t pb_decode_varint() const
        {
            if(value_==NULL) return 0;
            uint64_t value = 0;
            uint8_t pos = 0;
            uint8_t byte = 0;
            do{
                byte = ((uint8_t*)value_)[pos];
                value |= (uint64_t)(byte&0x7F) << pos*7;
                pos++;
            }while(byte>=0x80);
            return value;
        }

#ifdef ARDUINO
        void operator=(const String& str) {
            (*this) = str.c_str();
        }

        operator String(){
            return (const char*)(*this);
        }
#else
        void operator=(const std::string& str) {
            (*this) = str.c_str();
        }

        operator std::string(){
            return (const char*)(*this);
        }
#endif

    private:
        void* value_;
        field_type field_type_;

        template<class T>
        void set(T value) {
            if(allocate(sizeof(T))){
                memcpy(value_, &value, sizeof(T));
            }
        }
    };

    class pson_pair{
    private:
        char* name_;
        pson value_;
    public:
        pson_pair() : name_(NULL){
        }

        ~pson_pair(){
            pool.deallocate(name_);
        }

        void set_name(const char *name) {
            size_t name_size = strlen(name) + 1;
            if(allocate_name(name_size)){
                memcpy(name_, name, name_size);
            }
        }

        char* allocate_name(size_t size){
            name_ = (char*)pool.allocate(size);
            return name_;
        }

        pson& value(){
            return value_;
        }

        char* name() const{
            return name_;
        }
    };

    class pson_object : public pson_container<pson_pair> {
    public:

        pson &operator[](const char *name) {
            for(iterator it=begin(); it.valid(); it.next()){
                const char* item_name = it.item().name();
                if(item_name && strcmp(item_name, name)==0){
                    return it.item().value();
                }
            }
            if(pson_pair* pair = create_item()){
                pair->set_name(name);
                return pair->value();
            }else{
                static pson value;
                return value;
            }
        };
    };

    class pson_array : public pson_container<pson> {
    public:
        template<class T>
        pson_array& add(T item_value){
            pson* item = create_item();
            if(item!=NULL){
                *item = item_value;
            }
            return *this;
        }
    };

    inline pson::operator pson_object &() {
        if (field_type_ != object_field) {
            value_ = pool.allocate<pson_object>();
            field_type_ = value_ != NULL ? object_field : empty;
        }
        if(value_!=NULL && field_type_ == object_field){
            return *((pson_object *)value_);
        }else{
            static pson_object dummy;
            return dummy;
        }
    }

    inline pson::operator pson_array &() {
        if (field_type_ != array_field) {
            value_ = pool.allocate<pson_array>();
            field_type_ = value_!=NULL ? array_field : empty;
        }
        if(value_!=NULL && field_type_==array_field){
            return *((pson_array *)value_);
        }else{
            static pson_array dummy;
            return dummy;
        }
    }

    inline pson &pson::operator[](const char *name) {
        return ((pson_object &) *this)[name];
    }

    ////////////////////////////
    /////// PSON_DECODER ///////
    ////////////////////////////

    class pson_decoder {

    protected:
        size_t read_;

        virtual bool read(void* buffer, size_t size){
            read_+=size;
            return true;
        }

    public:

        pson_decoder() : read_(0) {

        }

        void reset(){
            read_ = 0;
        }

        size_t bytes_read(){
            return read_;
        }

        bool pb_decode_tag(pb_wire_type& wire_type, uint32_t& field_number)
        {
            uint32_t temp=0;
            if(!pb_decode_varint32(temp)) return false;
            wire_type = (pb_wire_type)(temp & 0x07);
            field_number = temp >> 3;
            return true;
        }

        bool pb_decode_varint32(uint32_t& varint){
            varint = 0;
            uint8_t byte;
            uint8_t bit_pos = 0;
            do{
                if(!read(&byte, 1) || bit_pos>=32){
                    return false;
                }
                varint |= (uint32_t)(byte&0x7F) << bit_pos;
                bit_pos += 7;
            }while(byte>=0x80);
            return true;
        }

        bool pb_decode_varint64(uint64_t& varint)
        {
            varint = 0;
            uint8_t byte;
            uint8_t bit_pos = 0;
            do{
                if(!read(&byte, 1) || bit_pos>=64){
                    return false;
                }
                varint |= (uint32_t)(byte&0x7F) << bit_pos;
                bit_pos += 7;
            }while(byte>=0x80);
            return true;
        }

        bool pb_skip(size_t size){
            uint8_t byte;
            bool success = true;
            for(size_t i=0; i<size && success; i++){
                success = read(&byte, 1);
            }
            return success;
        }

        bool pb_skip_varint(){
            uint8_t byte;
            bool success;
            do{
                success = read(&byte, 1);
            }while(byte>0x80 && success);
            return success;
        }

        bool pb_read_string(char *str, size_t size){
            if(str && read(str, size)){
                str[size]=0;
                return true;
            }
            return false;
        }

        bool pb_read_varint(pson& value)
        {
            uint8_t temp[10];
            uint8_t byte=0;
            uint8_t bytes_read=0;
            do{
                if(bytes_read==10 || !read(&byte, 1)) return false;
                temp[bytes_read] = byte;
                bytes_read++;
            }while(byte>=0x80);
            if(value.allocate(bytes_read)){
                memcpy(value.get_value(), temp, bytes_read);
                return true;
            }else{
                return false;
            }
        }

    public:

        bool decode(pson_object & object, size_t size){
            size_t start_read = bytes_read();
            while(size-(bytes_read()-start_read)>0){
                pson_pair* pair = object.create_item();
                if(pair==NULL || !decode(*pair)){
                    return false;
                }
            }
            return true;
        }

        bool decode(pson_array & array, size_t size){
            size_t start_read = bytes_read();
            while(size-(bytes_read()-start_read)>0){
                pson* item = array.create_item();
                if(item==NULL || !decode(*item)){
                    return false;
                }
            }
            return true;
        }

        bool decode(pson_pair & pair){
            uint32_t name_size;
            if(pb_decode_varint32(name_size)){
                return name_size != UINT32_MAX && pair.allocate_name(name_size + 1) && pb_read_string(pair.name(), name_size) && decode(pair.value());
            }
            return false;
        }

        bool decode(pson& value) {
            uint32_t field_number;
            pb_wire_type wire_type;
            if(!pb_decode_tag(wire_type, field_number)) return false;
            value.set_type((pson::field_type)field_number);
            if(wire_type==length_delimited){
                uint32_t size = 0;
                if(!pb_decode_varint32(size)) return false;
                switch(field_number){
                    case pson::string_field:
                        return size!=UINT32_MAX && value.allocate(size+1) && pb_read_string((char*)value.get_value(), size);
                    case pson::bytes_field: {
                        uint8_t varint_size = value.get_varint_size(size);
                        if(size<=UINT32_MAX-varint_size && value.allocate(size + varint_size)){
                            if(read((char*)value.get_value() + varint_size, size)){
                                value.pb_encode_varint(size);
                                return true;
                            }
                        }
                        return false;
                    }
                    case pson::object_field:
                        if(value.allocate<pson_object>()){
                            return decode(*(pson_object*) value.get_value(), size);
                        }
                        return false;
                    case pson::array_field:
                        if(value.allocate<pson_array>()){
                            return decode(*(pson_array*) value.get_value(), size);
                        }
                    default:
                        return false;
                }
            }else {
                switch (field_number) {
                    case pson::svarint_field:
                    case pson::varint_field:
                        return pb_read_varint(value);
                    case pson::float_field:
                        return value.allocate(4) && read(value.get_value(), 4);
                    case pson::double_field:
                        return value.allocate(8) && read(value.get_value(), 8);
                    case pson::null_field:
                    case pson::true_field:
                    case pson::false_field:
                    case pson::zero_field:
                    case pson::one_field:
                    case pson::empty_string:
                    case pson::empty_bytes:
                    case pson::empty:
                        return true;
                    default:
                        return false;
                }
            }
        }
    };

    ////////////////////////////
    /////// PSON_ENCODER ///////
    ////////////////////////////

    class pson_encoder {

    protected:
        size_t written_;

        virtual bool write(const void* buffer, size_t size){
            written_+=size;
            return true;
        }

    public:

        pson_encoder() : written_(0) {
        }

        void reset(){
            written_ = 0;
        }

        size_t bytes_written(){
            return written_;
        }

        void pb_encode_tag(pb_wire_type wire_type, uint32_t field_number){
            uint64_t tag = ((uint64_t)field_number << 3) | wire_type;
            pb_encode_varint(tag);
        }

        void pb_encode_varint(uint32_t field, uint64_t value)
        {
            pb_encode_tag(varint, field);
            pb_encode_varint(value);
        }

        uint8_t pb_write_varint(void * buffer)
        {
            uint8_t byte=0;
            uint8_t bytes_written=0;
            do{
                byte = *((uint8_t*)buffer + bytes_written);
                bytes_written++;
            }while(byte>=0x80);
            write(buffer, bytes_written);
            return bytes_written;
        }

        void pb_encode_varint(uint64_t value)
        {
            do
            {
                uint8_t byte = (uint8_t)(value & 0x7F);
                value >>= 7;
                if(value>0) byte |= 0x80;
                write(&byte, 1);
            }while(value>0);
        }

        void pb_encode_string(const char* str, uint32_t field_number){
            pb_encode_tag(length_delimited, field_number);
            pb_encode_string(str);
        }

        void pb_encode_string(const char* str){
            if(str!=NULL){
                size_t string_size = strlen(str);
                pb_encode_varint(string_size);
                write(str, string_size);
            }
        }

        template<class T>
        void pb_encode_submessage(T& element, uint32_t field_number)
        {
            pb_encode_tag(length_delimited, field_number);
            pson_encoder sink;
            sink.encode(element);
            pb_encode_varint(sink.bytes_written());
            encode(element);
        }

        void pb_encode_fixed32(void* value){
            write(value, 4);
        }

        void pb_encode_fixed64(void* value){
            write(value, 8);
        }

        void pb_encode_fixed32(uint32_t field, void*value)
        {
            pb_encode_tag(fixed_32, field);
            pb_encode_fixed32(value);
        }

        void pb_encode_fixed64(uint32_t field, void*value)
        {
            pb_encode_tag(fixed_64, field);
            pb_encode_fixed64(value);
        }

    public:

        void encode(pson_object & object){
            pson_container<pson_pair>::iterator it = object.begin();
            while(it.valid()){
                encode(it.item());
                it.next();
            }
        }

        void encode(pson_array & array){
            pson_container<pson>::iterator it = array.begin();
            while(it.valid()){
                encode(it.item());
                it.next();
            }
        }

        void encode(pson_pair & pair){
            pb_encode_string(pair.name());
            encode(pair.value());
        }

        void encode(pson & value) {
            switch (value.get_type()) {
                case pson::string_field:
                    pb_encode_string((const char*)value.get_value(), pson::string_field);
                    break;
                case pson::bytes_field:
                    pb_encode_tag(length_delimited, pson::bytes_field);
                    write(((const char *) value.get_value()) + pb_write_varint(value.get_value()), value.pb_decode_varint());
                    break;
                case pson::svarint_field:
                case pson::varint_field:
                    pb_encode_tag(varint, value.get_type());
                    pb_write_varint(value.get_value());
                    break;
                case pson::float_field:
                    pb_encode_fixed32(pson::float_field, value.get_value());
                    break;
                case pson::double_field:
                    pb_encode_fixed64(pson::double_field, value.get_value());
                    break;
                case pson::object_field:
                    pb_encode_submessage(*(pson_object *) value.get_value(), pson::object_field);
                    break;
                case pson::array_field:
                    pb_encode_submessage(*(pson_array *) value.get_value(), pson::array_field);
                    break;
                default:
                    pb_encode_tag(varint, value.get_type());
                    break;
            }
        }
    };
}

#endif