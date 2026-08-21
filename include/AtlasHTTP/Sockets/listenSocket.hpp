#ifndef ListenSocket_h
#define ListenSocket_h

#include <AtlasHTTP/Sockets/bindSocket.hpp>

namespace HTTP {
class ListenSocket : public BindSocket {
  private:
    int backlog;
    int listening;

  public:
    ListenSocket(int domain, int service, int protocol, int port, u_long interface, int bklg);
    void start_listening();
    int get_listening() const;
    int get_backlog() const;
};
} // namespace HTTP

#endif
