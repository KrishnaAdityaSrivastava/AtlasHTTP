#ifndef BindSocket_h
#define BindSocket_h

#include <stdio.h>

#include <AtlasHTTP/Sockets/socket.hpp>

namespace HTTP
{
    class BindSocket: public Socket{
        public:
            BindSocket( int domain, int service,int protocol, int port, u_long interface);
            int connect_network(int sock, struct sockaddr_in address);

    };
}

#endif