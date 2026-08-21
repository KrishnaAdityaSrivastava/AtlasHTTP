#include <AtlasHTTP/Sockets/bindSocket.hpp>

HTTP::BindSocket::BindSocket(int domain, int service, int protocol, int port, u_long interface)
    : Socket(domain, service, protocol, port, interface) {
    establish_connection();
}

int HTTP::BindSocket::connect_network(int sock, struct sockaddr_in address) {
    int opt = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }

    return bind(sock, (struct sockaddr *)&address, sizeof(address));
}