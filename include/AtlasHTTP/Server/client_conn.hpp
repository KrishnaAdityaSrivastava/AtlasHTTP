#pragma once
#include <string>


namespace HTTP{
    class ClientConnection{
        public:
        int fd;
        std::string read_buffer;
        std::string write_buffer;
        bool ready_to_write = false;
        bool keep_alive;
        size_t bytes_sent;
    };
}