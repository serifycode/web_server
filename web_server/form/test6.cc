#include "EventLoop.h"
#include "EventLoopThread.h"
#include <stdio.h>
#include <thread>

void runInThread()
{
    printf("runInThread(): pid = %d\n",
            std::this_thread::get_id());
}

int main()
{
    printf("main(): pid = %d\n",
            std::this_thread::get_id());

    EventLoopThread loopThread;
    EventLoop* loop = loopThread.startLoop();
    loop->runInLoop(runInThread);
    loop->quit();

    printf("exit main().\n");
}