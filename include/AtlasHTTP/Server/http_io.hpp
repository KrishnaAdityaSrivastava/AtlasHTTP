#pragma once

#include <string>
#include <cctype>
#include <fcntl.h>
#include <iostream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <filesystem>


#include <AtlasHTTP/Server/request.hpp>
#include <AtlasHTTP/Server/response.hpp>
#include <AtlasHTTP/Server/client_conn.hpp>

namespace HTTP::HttpIO {
        
    //std::string buffer;
    bool read_request_from_socket(int client_socket, Request& req);
    void send_response(int client_socket, const Response& res);
    void send_file_response(int client_socket, const std::string& path);

    bool parse_request(ClientConnection  &conn, Request &req);
} // namespace HTTP::HttpIO
