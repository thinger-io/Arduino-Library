#ifndef THINGER_CONSOLE_H
#define THINGER_CONSOLE_H

#include <Stream.h>
#include "ThingerClient.h"

#define BUFFER_SIZE     128
#define MAX_CMD_ARGS    10

#ifndef THINGER_PROMPT_COLOR
#define THINGER_PROMPT_COLOR "\033[1;34m"
#endif 

#ifndef TERMINAL_GREEN_COLOR
#define TERMINAL_GREEN_COLOR "\033[0;32m"
#endif 

#ifndef THINGER_RED_COLOR
#define THINGER_RED_COLOR "\033[1;31m"
#endif 

#ifndef THINGER_NORMAL_COLOR
#define THINGER_NORMAL_COLOR "\033[0m"
#endif

struct ThingerCommand{
    std::function<void(int argc, char* argv[])> callback;
    const char* description;
};


class ThingerConsole : public Stream{

public:

    ThingerConsole(ThingerClient& client) : 
        client_(client),
        resource_(client["$console"]),
        prompt_(client.get_device_id())
    {
        tx_buffer_[BUFFER_SIZE] = 0;

        register_commands();
        
        resource_ << [this](pson& in){
            if(!in.is_bytes()) return;

            // get input data
            size_t size = 0;
            const void * buffer = NULL;
            in.get_bytes(buffer, size);
            
            // iterate over received data
            for(auto i=0; i<size; ++i){

                // check received character
                char input = ((const uint8_t*)buffer)[i];

                // delete
                if(input==0x7F){ 
                    if(rx_index_>0){
                        rx_index_--;
                        remove_last_char();
                    }

                // ctrl +c
                }else if(input==0x03){ 
                    if(commands_enabled_){
                        printf("^C\r\n");
                        if(running_ && stop_cmd()){
                            running_ = false;
                        }else{
                            reset();
                            send_prompt_1();
                        }
                    }
                // ESC caracters
                }else if(input==0x1b){ // ESC
                    // do nothing on escape characters
                    i = size;
                    if(i+2<size){
                        char next = ((const uint8_t*)buffer)[++i];
                        if(next==0x5b){
                            char arrow = ((const uint8_t*)buffer)[++i];
                            switch(arrow){
                                case 'A': // up
                                break;
                                case 'B': // down
                                break;
                                case 'C': // right
                                break;
                                case 'D':  // left
                                break;
                                default:
                                break;
                            }
                        }
                    }
                
                // any other input
                }else{
                    // ensure we have remaining space in buffer
                    if(rx_index_>=BUFFER_SIZE){
                        THINGER_DEBUG("CONSOLE", "RX buffer is full...");
                        return;
                    }

                    // get previous character
                    char previous = rx_index_ > 0 ? rx_buffer_[rx_index_-1] : 0;
                    
                    // add character to the buffer
                    rx_buffer_[rx_index_++] = input;

                    // update quote state (if not scaped)
                    if(commands_enabled_ && input=='"' && previous!='\\' && !running_){
                        pending_quotation = !pending_quotation;
                    }

                    // new line
                    if(input==13){
                        
                        // send a \r\n
                        send_newline();

                        if(commands_enabled_ && !running_){
                                                            
                            // multiline prompt
                            if(previous=='\\' || pending_quotation){
                                return send_prompt_2();
                            }

                            // copy command from input buffer to command buffer (skip \n)
                            memcpy(cmd_buffer_, rx_buffer_, rx_index_-1);

                            // parse command arguments
                            char *argv[MAX_CMD_ARGS];
                            int argc = parse_command(cmd_buffer_, rx_index_-1, argv, MAX_CMD_ARGS);

                            // run command and arguments
                            if(argc){
                                THINGER_DEBUG_VALUE("CONSOLE", "command: ", argv[0]);
                                const char* cmd = argv[0];
                                auto* command = cmds_.find(cmd);
                                if(command==nullptr){
                                    printf("%s : " THINGER_RED_COLOR "command not found\r\n", argv[0]);
                                }else{
                                    run_cmd(command->callback, argc, argv);
                                }
                            }

                            reset();
                            send_prompt_1();
                        }else{
                            rx_input_ = rx_index_;
                        }
                        
                    // echo anything else
                    }else{
                        write(input);
                        flush();
                    }
                }
            }
        };

        resource_.set_io_type(thinger::thinger_resource::pson_stream);

        resource_.set_stream_listener([this](uint16_t, unsigned long, bool enabled){
            if(enabled){
                THINGER_DEBUG("CONSOLE", "Console started");
                console_running_ = true;
                reset();
                if(commands_enabled_){
                    send_prompt_1();
                }
            }else{
                console_running_ = false;
                THINGER_DEBUG("CONSOLE", "Console stopped");
            }
        });
    }
    
    virtual ~ThingerConsole(){
    }

    bool is_running(){
        return console_running_;
    }

    bool command_running(){
        client_.handle();
        return is_running() && running_;
    }
    
    size_t write(uint8_t value) override{
        tx_buffer_[tx_index_++] = value;
        if(value=='\n' || tx_index_==BUFFER_SIZE){
            flush();
        }
        return 1;
    }

    int available() override{
        client_.handle();
        return rx_input_ ? rx_input_ - rx_read_ : 0;
    }

    int read() override{
        if(!available()) return -1;
        auto value = rx_buffer_[rx_read_++];

        // reset buffer when input is read
        if(rx_read_>=rx_input_){
            reset();
        }

        return value;
    }

    int peek() override{
        if(!available()) return -1;
        return rx_buffer_[rx_read_];
    }

    void flush() override{
        if(tx_index_>0 && resource_.stream_enabled()){
            pson data;
            data.set_bytes(tx_buffer_, tx_index_);
            client_.stream_data(resource_, data);
        }
        tx_index_=0;
    }

    void set_prompt(const char* prompt){
        prompt_ = prompt;
    }

    /**
     * Boolean operator
     * @return true if the remote console is connected, false otherwise
     */
    operator bool(){
        return resource_.stream_enabled();
    }

    void command(const char* cmd, std::function<void(int argc, char* argv[])> callback, const char* desc = ""){
        commands_enabled_ = true;
        cmds_[cmd].callback = callback;
        cmds_[cmd].description = desc;
    }

    void error(const char* message){
        printf(THINGER_RED_COLOR "%s\r\n", message);
    }

protected:

    void register_commands(){
        
        command("clear", [&](int argc, char* argv[]){
            clear_screen();
        }, "clear console");

        command("reboot", [&](int argc, char* argv[]){
            println("device will reboot now...");
            client_.reboot();
        }, "reboot device");

        command("exit", [&](int argc, char* argv[]){
            // TODO, this protocol does not support closing streams
        }, "close terminal");

        command("help", [&](int argc, char* argv[]){
            auto cmd =  cmds_.begin();
            while(cmd){
                printf(TERMINAL_GREEN_COLOR "%s\t" THINGER_NORMAL_COLOR " - %s\r\n", cmd->key_, cmd->value_.description);
                cmd = cmd->next_;
            }
        }, "show this help");

        commands_enabled_ = false;
    }

    virtual bool run_cmd(std::function<void(int argc, char* argv[])>& callback, int argc, char* argv[]){
        reset();
        if(callback){
            running_ = true;
            callback(argc, argv);
            running_ = false;
        }
        return (bool)callback;
    }

     virtual bool stop_cmd(){
        return true;
    }

    void send_newline(){
        printf("\r\n");
    }

    void send_prompt_1(){
        if(prompt_!=nullptr && strlen(prompt_)>0){
            printf(THINGER_PROMPT_COLOR "%s", prompt_);
        }
        printf(THINGER_NORMAL_COLOR "$ ");
        flush();
    }

    void send_prompt_2(){
        printf(THINGER_NORMAL_COLOR "> ");
        flush();
    }

    void remove_last_char(){
        printf("\x08\x1b\x5b\x4a");
        flush();
    }

    void clear_screen(){
        printf("\x1b\x5b\x48\x1b\x5b\x4a");
        flush();
    }

    int parse_command(char* command, size_t size, char *argv[], size_t max_args){
        int argc = 0;
        char* current_arg = nullptr;
        bool open_quote = false;
        bool scape_next = false;

        static auto add_token = [&](size_t i){
            command[i] = 0;
            if(argc<max_args){
                argv[argc++] = current_arg;
            }
            current_arg = nullptr;
        };

        static auto move_left = [&](int& i){
            for(auto j=i; j<size-1; j++){
                command[j] = command[j+1];
            }
            command[--size] = 0;
            i--;
        };

        for(int i=0; i<size; ++i){
            char c = command[i];

            // scape next char
            if(scape_next){
                if(c=='\n' || c=='\r') move_left(i);
                else if(!current_arg) current_arg = &command[i];
                scape_next = false;
                continue;
            }

            // quote
            if(c=='"'){
                // closing previous quote
                if(open_quote) add_token(i);
                open_quote = !open_quote;
            }else if(c==' '){
                if(current_arg && !open_quote) add_token(i);
            }else if(c=='\\'){
                move_left(i);
                scape_next = true;
            }else if(!current_arg){
                current_arg = &command[i];
            }
        }

        if(current_arg) add_token(size);

        return argc;
    }

    void reset(){
        rx_index_ = 0;
        rx_read_ = 0;
        rx_input_ = 0;
        pending_quotation = false;
    }

protected:
    // thinger resources
    ThingerClient& client_;
    thinger::thinger_resource& resource_;
    const char* prompt_ = nullptr;

    // tx buffer
    uint8_t tx_buffer_[BUFFER_SIZE+1];
    size_t tx_index_ = 0;

    // rx buffer
    uint8_t rx_buffer_[BUFFER_SIZE+1];
    size_t rx_index_  = 0;
    size_t rx_read_   = 0;
    size_t rx_input_  = 0;

    // buffer for holding args for commands
    char cmd_buffer_[BUFFER_SIZE];

    // flags for console parser
    bool pending_quotation = false;

    // flags for console operation
    bool commands_enabled_ = false;
    bool running_ = false;
    bool console_running_ = false;

    // structure for holding commands
    thinger_map<ThingerCommand> cmds_;

};

#endif
