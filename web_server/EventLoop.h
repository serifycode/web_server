#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include "Timestamp.h"
#include <thread>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

class Channel;
class Poller;

class EventLoop
{
public:
    typedef std::function<void()> Functor;
    typedef std::vector<Channel*> ChannelList;
    typedef std::function<void(Timestamp)> Callback;
    EventLoop();
    ~EventLoop();

    void AddChannelToPoller(Channel* pchannel);


    //移除事件
    void RemoveChannelToPoller(Channel* pchannel);


    //修改事件
    void UpdateChannelToPoller(Channel* pchannel);

    void runInLoop(const Functor& cb);
    void queueInLoop(const Functor& cb);

    void loop();
    void quit();

    void wakeup();

    void runafter(double delay,Callback cb);

    void runevery(double intervel,Callback cb);

    void Timerhandle(Channel* timechannel,bool once,Timestamp time_);
    void SetTimerback(const Callback& cb) {Timerback_=cb;}

    bool isInLoopThread() const { return threadId_==std::this_thread::get_id();}
    std::thread::id Getthreadid() const { return threadId_;}
private:
    void handleRead(); //wake up
    void doPendingFunctors();//执行函数，不暴露其他线程

    bool looping_;
    bool quit_;
    bool callingPendingFunctors_;//正在执行？

    Timestamp pollReturnTime_;

    std::unique_ptr<Poller> poller_;//用智能指针控制时间

    int wakeup_;//唤醒

    std::unique_ptr<Channel> wakeupChannel_; //监听唤醒

    std::mutex mutex_;

    std::vector<Functor> pendingFunctors_;

    Callback Timerback_;

    std::thread::id threadId_;
    ChannelList activeChannels_;
};

#endif