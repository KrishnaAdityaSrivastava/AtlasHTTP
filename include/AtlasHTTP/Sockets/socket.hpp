#ifndef Socket_h
#define Socket_h

#include <netinet/in.h>
#include <sys/socket.h>

namespace HTTP {
class Socket {
  private:
    struct sockaddr_in address{};
    int sock{-1};
    int connection{-1};

  protected:
    void establish_connection();

  public:
    Socket(int domain, int service, int protocol, int port, u_long interface);
    virtual ~Socket() = default;

    virtual int connect_network(int sock, struct sockaddr_in address) = 0;
    void test_connection(int test_obj) const;

    struct sockaddr_in get_address() const;
    int get_sock() const;
    int get_connection() const;

    void set_connection(int con);
};
} // namespace HTTP

#endif
