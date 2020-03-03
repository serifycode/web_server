#ifndef CHANNEL_H
#define CHANNEL_H

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>

class EventLoop;

class Channel:noncopyable
{
public:
    typedef std::function<void()> Callback;
    typedef std::function<void(Timestamp)> ReadCallback;
    
    Channel();
    ~Channel();

    int SetFd(int fd)
    {
        fd_=fd;
    }
    
    int GetFd() const
    {
        return fd_;
    }

    void SetEvents(uint32_t events)
    {
        events_=events;
    }

    uint32_t GetEvents() const
    {
        return events_;
    }

    void HandleEvent(Timestamp receivetime);

    void SetReadCallback(const ReadCallback& cb)
    {
        readCallback_=cb;
    }
    
    void SetWriteCallback(const Callback& cb)
    {
        writeCallback_=cb;
    }

    void SetErrorCallback(const Callback& cb)
    {
        errorCallback_=cb;
    }

    void SetCloseCallback(const Callback& cb)
    {
        closeCallback_=cb;
    }


private:
    int fd_;//连接符
    uint32_t events_;

    ReadCallback readCallback_;
    Callback writeCallback_;
    Callback errorCallback_;
    Callback closeCallback_;

    

};
#endif