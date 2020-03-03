#include "TcpServer.h"
#include "EventLoop.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"

#include <functional>
#include <iostream>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

void Setnonblocking(int socketfd_)
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

TcpServer::TcpServer(EventLoop* loop,int port,int num):
                            loop_(loop),//监听建立连接线程
                            socket_(),
                            serverchannel_(new Channel()),
                            iothreadnum(num),
                            eventLoopThreadpool_(new EventLoopThreadPool(loop_,iothreadnum))                                   
{
    socket_.SetReuseAddr();
    socket_.BindAddress(port);
    socket_.Setnonblocking();
    socket_.Listen();
    serverchannel_->SetFd(socket_.fd());
    serverchannel_->SetReadCallback(std::bind(&TcpServer::NewConnection, this,std::placeholders::_1));
    serverchannel_->SetEvents(EPOLLIN | EPOLLET);
}

TcpServer::~TcpServer()
{

}

void TcpServer::start()
{
    std::cout<<"main thread for listen start:id:"<<loop_->Getthreadid()<<std::endl;
    //刚创建单线程可直接调用
    //loop_->runInLoop(std::bind(&EventLoop::AddChannelToPoller,loop_,serverchannel_));
    loop_->AddChannelToPoller(serverchannel_);
    eventLoopThreadpool_->start();//开启IO线程
    std::cout<<iothreadnum<<"IOthread for client start"<<std::endl;
}

void TcpServer::NewConnection(Timestamp receivetime)
{
    struct sockaddr_in clientaddr;
    int clientfd;
    while( (clientfd = socket_.Accept(clientaddr)) > 0) 
    { 
       // std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
        //    << ":" << ntohs(clientaddr.sin_port) << std::endl;
        Setnonblocking(clientfd);
        EventLoop* ioloop=eventLoopThreadpool_->GetNextLoop();
        TcpConnectionPtr connectionPtr(new TcpConnection(ioloop,clientfd,clientaddr));
        connections_[clientfd]=connectionPtr;
        connectionPtr->setConnectionCallback(connectionCallback_);
        connectionPtr->setMessageCallback(messageCallback_);
        connectionPtr->setCloseCallback(std::bind(&TcpServer::RemoveConnection,
        this,std::placeholders::_1));//回调必然在IO线程，但删除connetion_在主线程
        ioloop->queueInLoop(std::bind(&TcpConnection::connectEstablished,connectionPtr,receivetime));//将对应channel加到loopread，需要在IO线程完成
    }
    

}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  loop_->runInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn)//主要是为了关闭时把server对应的conn删除才回调
{
    connections_.erase(conn->fd());//已经处于主线程
    //EventLoop* IoLoop=conn->GetIoLoop();
    //IoLoop->queueInLoop(         //将对应channel从loopread删除
     // std::bind(&TcpConnection::connectDestroyed, conn));//必不是位于IO线程，直接加到队列中
                                                            //少一次判断
}