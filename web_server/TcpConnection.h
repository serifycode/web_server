#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "Timestamp.h"
#include "noncopyable.h"
#include <memory>
#include <functional>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/any.hpp>


class EventLoop;
class Channel;

class TcpConnection:noncopyable,public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void(const TcpConnectionPtr&)> Callback;  
    typedef std::function<void(const TcpConnectionPtr&,Timestamp)> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&,
                            const std::string&,Timestamp)> MessageCallback;
    
    TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr);
    ~TcpConnection();

    void setContext(const boost::any& context)
    { context_ = context; }

    const boost::any& getContext() const
    { return context_; }

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setCloseCallback(const Callback& cb)
    { closeCallback_ = cb; }

    void SetSendCompleteCallback(const Callback &cb)
    { sendcompletecallback_ = cb;}

    void handleRead(Timestamp receivetime);
    void handleWrite();
    void handleClose();
    void handleError();


    void connectEstablished(Timestamp receivetime);//给Tcpserver 调用
    void connectDestroyed();

    
    void Send(const std::string& buffer);
    void sendInLoop();
    void Shutdown();
    void shutdownInLoop();

    bool connected() const 
    { return state_==kConnected;}
    int fd() const 
    { return fd_;}
    struct sockaddr_in GetclientAddress() const 
    { return clientaddr_;}

    EventLoop* GetIoLoop() const 
    { return loop_;}

private:
    bool iswriting;

    enum StateE { kConnecting, kConnected,kDisconnecting, kDisconnected, };
    StateE state_;
    void setState(StateE s) { state_ = s; }

    EventLoop* loop_;
    int fd_;
    struct sockaddr_in clientaddr_;
    std::unique_ptr<Channel> ChannelPtr;
    ConnectionCallback connectionCallback_;
    Callback closeCallback_;
    Callback sendcompletecallback_;
    MessageCallback messageCallback_;
    std::string bufferout_;
    std::string bufferin_;
    int sendn(int fd,std::string& bufferout);
    int readn(int fd,std::string& bufferin);
    
    boost::any context_;

};
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;

#endif