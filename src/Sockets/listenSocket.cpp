#include <AtlasHTTP/Sockets/listenSocket.hpp>

HTTP::ListenSocket::ListenSocket(int domain, int service, int protocol, int port, u_long interface, int bklg)
    : BindSocket(domain, service, protocol, port, interface), backlog(bklg), listening(-1) {
    start_listening();
    test_connection(listening);
}

void HTTP::ListenSocket::start_listening() { listening = listen(get_sock(), backlog); }

int HTTP::ListenSocket::get_listening() const { return listening; }

int HTTP::ListenSocket::get_backlog() const { return backlog; }
