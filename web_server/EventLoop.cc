#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/epoll.h>
#include "assert.h"
#include <iostream>
#include <thread>
#include <vector>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

static int createEventfd()
{
    int evefd=::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evefd < 0)
    {
        std::cout << "Failed in eventfd" <<std::endl;
        abort();
    }
    return evefd;
}

class IgnoreSigPipe//对于未及时处理客户端的断开，继续向客户端写入
{
 public:
    IgnoreSigPipe()
    {
        signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
    :   looping_(false),
        quit_(false),
        callingPendingFunctors_(false),
        threadId_(std::this_thread::get_id()),
        poller_(new Poller()),
        wakeup_(createEventfd()),
        wakeupChannel_(new Channel())

{
    wakeupChannel_->SetFd(wakeup_);
    wakeupChannel_->SetEvents(EPOLLIN | EPOLLET);
    wakeupChannel_->SetReadCallback(std::bind(&EventLoop::handleRead,this));//bind记得&
    this->AddChannelToPoller(wakeupChannel_.get());
    //std::cout<<"EventLoop created"<<threadId_<<std::endl;
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    close(wakeup_);
}

void EventLoop::AddChannelToPoller(Channel *pchannel)
{
    poller_->AddChannel(pchannel);
}

//移除事件
void EventLoop::RemoveChannelToPoller(Channel *pchannel)
{
    poller_->RemoveChannel(pchannel);
}

//修改事件
void EventLoop::UpdateChannelToPoller(Channel *pchannel)
{
    poller_->UpdateChannel(pchannel);
}

void EventLoop::loop()
{
    assert(!looping_);
    looping_ = true;
    quit_ = false;
    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_=poller_->poll(activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            (*it)->HandleEvent(pollReturnTime_);
        }
        doPendingFunctors();

    }
    std::cout<<"EventLoop"<<" stop looping"<<std::endl;
    looping_=false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
  // wakeup();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_, &one, sizeof one);
    if (n != sizeof one)
    {
        std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8"<<std::endl;
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_, &one, sizeof one);
    if (n != sizeof one)
    {
        std::cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8"<<std::endl;
    }
}

void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)//主线程或当前线程都可能添加到任务队列，应互斥
{                                             //我觉得可以全部都用runinloop
    {
        std::lock_guard <std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);//和前面互锁，主线程放入任务队列时锁住
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::runafter(double delay,Callback cb)
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
    struct timespec time;
    time.tv_sec=delay/1;
    time.tv_nsec=(delay-time.tv_sec)*1000*1000; 
    struct itimerspec timer;
    timer.it_interval={0,0};
    timer.it_value=time;
    timerfd_settime(timerfd,0,&timer,0);
    Channel* timechannel=new Channel();
    timechannel->SetFd(timerfd);
    timechannel->SetEvents(EPOLLIN | EPOLLET);
    timechannel->SetReadCallback(std::bind(&EventLoop::Timerhandle,this,
                                        timechannel,true,std::placeholders::_1));
    this->AddChannelToPoller(timechannel);
}

void EventLoop::runevery(double intervel,Callback cb)
{
    SetTimerback(cb);
    int timerfd = timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
    struct timespec time;
    time.tv_sec=intervel/1;
    time.tv_nsec=(intervel-time.tv_sec)*1000*1000; 
    struct itimerspec timer;  
    timer.it_interval=time;
    timer.it_value=time;
    timerfd_settime(timerfd,0,&timer,NULL);
    Channel* timechannel=new Channel();
    timechannel->SetFd(timerfd);
    timechannel->SetEvents(EPOLLIN | EPOLLET);
    timechannel->SetReadCallback(std::bind(&EventLoop::Timerhandle,this,
                                        timechannel,false,std::placeholders::_1));
    this->AddChannelToPoller(timechannel);
}

void EventLoop::Timerhandle(Channel* timechannel,bool once,Timestamp time_)
{
    
    uint64_t howmany;	
    if (read(timechannel->GetFd(), &howmany, sizeof(howmany)) != sizeof(howmany))//设置RT模式，定时器到
    {                                                           //必须读，否则不会触发下一次
        std::cout<<"read error"<<std::endl;
        return;
    }
    Timerback_(time_);
    if(once)//一次定时，移出loop，删除fd
    {
        this->RemoveChannelToPoller(timechannel);
        close(timechannel->GetFd());
        delete timechannel;
    }
}
