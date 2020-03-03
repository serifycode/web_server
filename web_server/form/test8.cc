//仿照8.5节写的echo服务器，主线程只接收客户端发来的连接，把newconnectin
//放到另一个线程中，用eventloopthread开的，处理已连接客服端的消息，写的echo
//服务。

//另外添加了关闭事件



#include "TcpServer.h"
#include "EventLoop.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>


void onConnection(const TcpConnectionPtr& con)
{
    struct sockaddr_in clientaddr=con->GetclientAddress();
    std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
        << ":" << ntohs(clientaddr.sin_port) << std::endl;
}

void onMessage(const TcpConnectionPtr& con,const std::string& ss)
{
    struct sockaddr_in clientaddr=con->GetclientAddress();
    std::cout <<"receive from IP"<<inet_ntoa(clientaddr.sin_addr) 
        << ":" << ntohs(clientaddr.sin_port)<<"messege:"
        <<ss.c_str()<<std::endl;
    write(con->fd(), ss.c_str(), sizeof(ss.c_str()));
}

int main()
{
    std::cout<<"mainid:"<<std::this_thread::get_id()<<std::endl;

    EventLoop loop;

    TcpServer server(&loop, 80);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    loop.loop();
}