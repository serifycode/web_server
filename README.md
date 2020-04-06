# *web_server*
参考muduo、Netserver、webserver写的服务器

## **时间轮定时器踢掉不活跃的连接：**
   假设时间长度为8s，8s未接收到消息就shutdown，每秒钟内，将所有连接或者有消息到达的Tcpconnetion的剩余保活期重置为7，并每秒断开保活期为0的连接。
具体做法是维护一个大小为7的队列，队列中每个元素为一个unordered_set。每当连接建立时，以Tcpconnection的弱引用初始化类实体Entry，将Entry的weak_ptr加到Tcpconnection中，将并将Entry的shared_ptr加到队列末尾的哈希中，并且每当消息到来时，取出Tcpconnection的弱引用，如果存在转成强引用，并加到队尾的哈希中。
   每次丢弃的哈希都会是对应的引用计数减一，当引用计数位0的，Entry析构，将Tcpconnection试图强化，并shutdow。
另：可将队列放入主线程中，可实现多线程的kickout不活跃连接。

## **将所有事件都文件化，一致统一用epoll处理**
1. 主线程loop的listen文件描述符。
2. IO线程的使用eventfd,主线程跨线程分配任务时可能处于epoll_wait，实现跨线程唤醒。
3. 定时器使用timerfd，需要定时完成或者周期性完成。(注意超时后要读，否则下一次超时不提醒。
4. 已连接的客服端fd。
所有事件统一放到epoll中，通过设定不同的读回调，处理不同的事务。

## **优雅关闭连接**
建立连接比较简单，只需要accept，但要处理关闭要分很多情况：
1. 对于read：
   返回值大于0，正常；等于0，对端关闭，直接close；
   小于0，需要分error：EAGIAN：读缓冲区为空，可正常返回；
   EINTR：被打断，不用管，继续读；
   其他错误，大概率RST，直接close。
2. 对于write：
   返回值大于0，正常；等于0，直接close；
   小于0，需要分error：EAGIAN：写缓冲区满，可正常返回；
   EINTR：被打断，不用管，继续读；
   EPIPE，对端已经close，可能是RST，直接close；
   其他错误直接close。
   对端如果发来FIN，不能直接关闭，应该shutdown，关闭写端，这里则需要判断，如果当前正在写，则要等全部写到缓冲区后，再shutdown写端。一般shutdown调用更快，所以需要设置状态disconnecting，让写事件完成时，再回调shutdownInLoop，因为写事件有多个，所以保留一个变量，直到等于0后才调用。


## **网络模型**
该服务器是基于reator模型，NIO复用，加线程池。采用epoll的ET模式。
