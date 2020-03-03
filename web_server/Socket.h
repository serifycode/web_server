#ifndef SOCKET_H
#define SOCKET_H

#include "noncopyable.h"
#include <arpa/inet.h>

class Socket : noncopyable
{
public:
    Socket();
    ~Socket();

    void BindAddress(const int& port);
    void Listen();
    int Accept(struct sockaddr_in &clientaddr);
    void Setnonblocking();
    void SetReuseAddr();
    int fd() const { return socketfd_; } 

private:
    int socketfd_;
};


#endif