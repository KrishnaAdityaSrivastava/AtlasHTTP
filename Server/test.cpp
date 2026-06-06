#include <stdio.h>

#include "server.hpp"
#include "request.hpp"
#include "response.hpp"

#include "../Cache/cache.hpp"
#include <signal.h>


int main(){
    signal(SIGPIPE, SIG_IGN);

    HTTP::Server server(AF_INET,SOCK_STREAM,0,8080,INADDR_ANY,16,4);

    server.get("/", [](const HTTP::Request& req) {
        HTTP::Response res(200, "<h1>Hello, World!</h1>");
        res.headers["Content-Type"] = "text/html";
        
        return res;
    });

    // server.get("/about", [](const HTTP::Request& req) {
    //     return HTTP::Response(200, "About page");
    // });

    server.get("/data/:id", [&server](const HTTP::Request& req) {
        std::string id = req.params.at("id");
        HTTP::Response res;
        res.is_file = true;
        res.file_path = id;

        //std::cout << "ID: " << id << std::endl;
        //std::cout << "PATH: " << res.file_path << std::endl;
        //res.headers["Content-Type"] = "text/html";
        return res;
    });

    // server.get("/users/:id", [&server](const HTTP::Request& req) {
    //     std::string id = req.params.at("id");
    //     server.get_cache().put("id", std::stoi(id));
    //     return HTTP::Response(200, std::to_string(server.get_cache().get("id")));
    // });

    // server.post("/", [](const HTTP::Request& req) {
    //     return HTTP::Response(201, "Received: " + req.body);
    // });

    server.launch();
}