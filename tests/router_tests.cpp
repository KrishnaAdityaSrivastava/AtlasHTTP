#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include <AtlasHTTP/Server/router.hpp>

int main() {
    std::unordered_map<std::string, std::string> params;
    assert(HTTP::Router::match_path("/users/:id", "/users/42", params));
    assert(params.at("id") == "42");

    params.clear();
    assert(!HTTP::Router::match_path("/users/:id", "/projects/42", params));

    std::vector<HTTP::Route> routes;
    routes.push_back(HTTP::Route{"GET", "/hello/:name", [](const HTTP::Request& req) {
                         return HTTP::Response(200, "Hello, " + req.params.at("name"));
                     }});

    HTTP::Request req;
    req.route.method = "GET";
    req.route.path = "/hello/atlas";

    const HTTP::Response res = HTTP::Router::dispatch(routes, req);
    assert(res.status == 200);
    assert(res.body == "Hello, atlas");
    assert(req.params.at("name") == "atlas");

    return 0;
}
