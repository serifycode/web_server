#include "Socket.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <cstring>

Socket::Socket()
{
    socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == socketfd_)
    {
        perror("socket create fail!");
        exit(-1);
    }
}

Socket::~Socket()
{
    close(socketfd_);
}

void Socket::BindAddress(const int& port)
{
    struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr(_ServerIP.c_str());
	serveraddr.sin_port = htons(port);
	int resval = bind(socketfd_, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (resval == -1)
	{
		close(socketfd_);
		perror("error bind");
		exit(1);
	}
}

void Socket::Listen()
{
    if (listen(socketfd_, 8192) < 0)
	{
		perror("error listen");
		close(socketfd_);
		exit(1);
	}
}

int Socket::Accept(struct sockaddr_in &clientaddr)
{
    socklen_t lengthofsockaddr = sizeof(clientaddr);
    int clientfd = accept(socketfd_, (struct sockaddr*)&clientaddr, &lengthofsockaddr);
    if (clientfd < 0) 
    {
        if(errno == EAGAIN)
            return 0;
        return clientfd;
	}
    return clientfd;
}

void Socket::Setnonblocking()
{
      // non-block
  int flags = ::fcntl(socketfd_, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(socketfd_, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(socketfd_, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(socketfd_, F_SETFD, flags);
  // FIXME check
}

void Socket::SetReuseAddr()
{
    int on = 1;
    setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

