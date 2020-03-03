#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>

Socket* psocket;

void newConnection()
{
    struct sockaddr_in clientaddr;
    int clientfd;
    while( (clientfd = psocket->Accept(clientaddr)) > 0) 
    {
        std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
            << ":" << ntohs(clientaddr.sin_port) << std::endl;
        write(clientfd, "How are you?\n", 13);
        close(clientfd);
    }
}



int main()
{
    std::cout<<"mainid:"<<std::this_thread::get_id()<<std::endl;
    EventLoop loop;
    Socket socket;
    psocket=&socket;
    socket.BindAddress(80);
    socket.Setnonblocking();
    socket.Listen();
    Channel* channel=new Channel;
    channel->SetFd(socket.fd());
    channel->SetReadCallback(newConnection);
    channel->SetEvents(EPOLLIN | EPOLLET);
    loop.AddChannelToPoller(channel);

    loop.loop();
}