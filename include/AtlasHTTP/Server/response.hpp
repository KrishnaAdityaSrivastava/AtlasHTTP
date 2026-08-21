#pragma once
#include <string>
#include <unordered_map>

namespace HTTP {

class Response {
    public:
        int status = 404;
        std::string body;

        std::unordered_map<std::string, std::string> headers;

        bool is_file = false;
        std::string file_path;

        Response(int status = 200, std::string body = "") : status(status), body(body) {}
    };

}