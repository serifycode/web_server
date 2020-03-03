#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* mainloop,int num):
                    mainloop_(mainloop),threadnum(num),next(0)
{
    for(int i=0;i<threadnum;i++)
    {
        EventLoopThread* thread= new EventLoopThread();
        threadlist_.push_back(thread);
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    for(int i=0;i<threadnum;i++)
    {
        delete threadlist_[i];
    }
}

void EventLoopThreadPool::start()
{
    for(int i=0;i<threadnum;i++)
    {
        looplist_.push_back(threadlist_[i]->startLoop());
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop()
{
    EventLoop* ioloop=mainloop_;
    if(looplist_.size()!=0)
    {
        ioloop=looplist_[next];
        next++;
        if(next==threadnum)
            next=0;
    }
    return ioloop;
}


