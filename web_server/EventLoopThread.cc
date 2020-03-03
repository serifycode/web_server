#include "EventLoopThread.h"
#include "EventLoop.h"
#include <functional>

EventLoopThread::EventLoopThread()
  : loop_(NULL),
    thread_(std::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_()
{
}

EventLoopThread::~EventLoopThread()
{
    loop_->quit();
    thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
    {
        std::unique_lock <std::mutex> lock(mutex_);
        while (loop_ == NULL)
        {
            cond_.wait(lock);
        }
    }

  return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    {
        std::lock_guard <std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();;
    }

    loop.loop();
    //assert(exiting_);
}