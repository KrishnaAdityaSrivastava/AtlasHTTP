#ifndef Server_h
#define Server_h

#include <functional>
#include <string>
#include <vector>

#include "thread_pool.hpp"

#include "request.hpp"
#include "response.hpp"
#include "route.hpp"

#include "../network.hpp"
#include "client_conn.hpp"

namespace HTTP {
class Server {
  private:
    ListenSocket* socket;

    int accepter();
    //void process_request(int client_socket, Request& req);
    
    HTTP::ThreadPool thread_pool;

    std::function<std::string(Request)> custom_handler;
    std::vector<Route> routes;
    HTTP::Cache cache;

    std::unordered_map<int, ClientConnection> conn_map;

  public:
    Server(int domain, int service, int protocol, int port, u_long interface, int bklg,int threads_count);

    ListenSocket* get_socket();
    void handle_client_connection(int client_socket);

    void processRequest(Request& req);

    //void set_handler(std::function<std::string(Request)> operation);

    void get(const std::string& path, std::function<HTTP::Response(const Request&)> handler);
    void post(const std::string& path, std::function<HTTP::Response(const Request&)> handler);

    std::string make_key(const std::string& method, const std::string& path);

    void launch();
};
} // namespace HTTP

#endif
