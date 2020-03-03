#include "EventLoop.h"
#include "Channel.h"

#include <iostream>
#include <thread>
#include <stdio.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <string.h>



EventLoop* g_loop;

void timeout()
{
    printf("Timeout!\n");
    g_loop->quit();
}

int main()
{
    EventLoop loop;
    g_loop = &loop;

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel;
    channel.SetFd(timerfd);
    channel.SetReadCallback(timeout);
    channel.SetEvents(EPOLLIN | EPOLLET);
    g_loop->AddChannelToPoller(&channel);
    struct itimerspec howlong;
    memset(&howlong, 0,sizeof howlong);
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();

    //close(timerfd);
    return 0;
}