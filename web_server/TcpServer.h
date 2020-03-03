#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "TcpConnection.h"
#include "Socket.h"
#include "EventLoopThread.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <map>
#include <memory>


class EventLoop;
class Channel;
class EventLoopThreadPool;


class TcpServer:noncopyable
{
public:
    typedef std::function<void(const TcpConnectionPtr&,Timestamp)> ConnectionCallback;  
    typedef std::function<void (const TcpConnectionPtr&,
                            const  std::string&,Timestamp)> MessageCallback;

    TcpServer(EventLoop* loop,int port,int num=0);
    ~TcpServer();

    void start();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

private:
    void NewConnection(Timestamp receivetime);
    void RemoveConnection(const TcpConnectionPtr& conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr& conn);
    typedef std::map<int, TcpConnectionPtr> ConnectionMap;
    Socket socket_;
    Channel* serverchannel_;
    EventLoop* loop_; //主线程
    int iothreadnum;
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadpool_;//注意loop_和iothreadnum在这之前
    ConnectionMap connections_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_ ;
};



#endif