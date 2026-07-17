#include "server.hpp"

#include <iostream>
#include <thread>
#include <unistd.h>

#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#include "http_io.hpp"
#include "router.hpp"

HTTP::Server::Server(int domain, int service, int protocol, int port, u_long interface, int bklg,int threads_count) {
    socket = new ListenSocket(domain, service, protocol, port, interface, bklg);
    if (threads_count > 0) thread_pool.init(threads_count);
    else thread_pool.init(std::thread::hardware_concurrency());
}

HTTP::ListenSocket* HTTP::Server::get_socket() { return socket; }

void HTTP::Server::get(const std::string& path, std::function<HTTP::Response(const Request&)> handler) {
    routes.push_back(Route{"GET", path, handler});
}

void HTTP::Server::post(const std::string& path, std::function<HTTP::Response(const Request&)> handler) {
    routes.push_back(Route{"POST", path, handler});
}

int HTTP::Server::accepter() {
    struct sockaddr_in address = get_socket()->get_address();
    int addrlen = sizeof(address);

    int fd =  accept(get_socket()->get_sock(), (struct sockaddr*)&address, (socklen_t*)&addrlen);

    if (fd < 0) return -1;
    
    ClientConnection conn;
    conn.fd = fd;
    conn_map[fd] = conn;

    return fd;
}

void HTTP::Server::handle_client_connection(int client_socket) {
    struct timeval tv{5, 0};
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    while(true){
        Request req;
        if (!HTTP::HttpIO::read_request_from_socket(client_socket, req)) {
            break;
        }
        HTTP::Response res = HTTP::Router::dispatch(routes, req);
        
        if (res.is_file) {
            HTTP::HttpIO::send_file_response(client_socket, res.file_path);
            close(client_socket);
            return;
        }
        else HTTP::HttpIO::send_response(client_socket, res);

        auto it = req.headers.find("Connection");
        if (it != req.headers.end() && it->second == "close") {
            break;
        }
    }
    close(client_socket);
    
    //cache.inc("reqcnt", 1);
}

void HTTP::Server::launch() {
    int listen_fd = socket->get_sock();

    // make listening socket non-blocking
    fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL, 0) | O_NONBLOCK);

    int epfd = epoll_create1(0);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    epoll_event events[64];

    while (true) {
        int nfds = epoll_wait(epfd, events, 64, -1);

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                // accept all pending connections
                while (true) {
                    int client_fd = accepter();
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        break;
                    }

                    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);

                    epoll_event cev{};
                    cev.events = EPOLLIN;
                    cev.data.fd = client_fd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
                }
            } else {
                // read available data
                char buf[4096];
                while (true) {
                    auto it = conn_map.find(fd);
                    if (it == conn_map.end()) break;
                    auto& conn = it->second;

                    char buf[4096];

                    ssize_t n = read(fd, buf, sizeof(buf));
                    
                    if (n > 0)  conn.read_buffer.append(buf, n);
                    else if (n <= 0) {
                        if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                            conn_map.erase(fd);
                            close(fd);
                        }
                        break;
                    }
                       
                    Request req;
                    while (HTTP::HttpIO::parse_request(conn, req)) {

                        thread_pool.enqueue([this,fd, req]() mutable {
                            auto it = conn_map.find(fd);
                            if (it == conn_map.end()) return;
                            
                            Response res = Router::dispatch(routes, req);

                            std::string response_str;
                            if (res.is_file) {
                                HTTP::HttpIO::send_file_response(fd, res.file_path);
                            } else {
                                HTTP::HttpIO::send_response(fd,res);
                            }
                        });
                    }
                }
                //std::cout << buf;
            }
        }
    }
}

std::string HTTP::Server::make_key(const std::string& method, const std::string& path) {
    return method + ":" + path;
}
