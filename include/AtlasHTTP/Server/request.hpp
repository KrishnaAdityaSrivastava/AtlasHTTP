#pragma once
#include <string>
#include <AtlasHTTP/Server/route.hpp>
#include <unordered_map>

namespace HTTP {

struct Request {
    HTTP::Route route;
    std::string version;
    std::string body;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> headers;
};

}