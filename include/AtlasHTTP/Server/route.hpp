#pragma once
#include <string>
#include <functional>
#include <AtlasHTTP/Server/response.hpp>

namespace HTTP {
struct Request;
struct Route {
    std::string method;
    std::string path;
    std::function<HTTP::Response(const Request&)> handler;
};

}