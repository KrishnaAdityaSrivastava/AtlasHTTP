#ifndef ConnectSocket_h
#define ConnectSocket_h

#include <stdio.h>

#include <AtlasHTTP/Sockets/socket.hpp>

namespace HTTP
{
    class ConnectSocket: public Socket{
        public:
            ConnectSocket( int domain, int service,int protocol, int port, u_long interface);
            int connect_network(int sock, struct sockaddr_in address);

    };
}

#endif