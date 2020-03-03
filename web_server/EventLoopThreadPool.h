#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include "noncopyable.h"

#include <vector>

class EventLoopThread;
class EventLoop;

class EventLoopThreadPool:noncopyable
{
public:
    EventLoopThreadPool(EventLoop* mainloop,int num=0);
    ~EventLoopThreadPool();
    void start();
    EventLoop* GetNextLoop();

private:
    EventLoop* mainloop_;//主线程循环
    int threadnum;
    int next;
    std::vector<EventLoopThread*> threadlist_;
    std::vector<EventLoop*> looplist_;

};

#endif 