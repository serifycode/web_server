#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include <functional>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#define BUFFSIZE 1000

int readevent=0;//收集所有写事件，都完成后才可shutdown

TcpConnection::TcpConnection(EventLoop *loop, 
                            int fd, 
                            const struct sockaddr_in &clientaddr)
                            :loop_(loop),
                            iswriting(false),
                            state_(kConnecting),
                            fd_(fd),
                            clientaddr_(clientaddr),
                            ChannelPtr(new Channel())
{
    ChannelPtr->SetFd(fd_);
    ChannelPtr->SetEvents(EPOLLIN | EPOLLET);
    ChannelPtr->SetReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));//构造时用this指针
    ChannelPtr->SetWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    ChannelPtr->SetErrorCallback(std::bind(&TcpConnection::handleError,this));
    ChannelPtr->SetCloseCallback(std::bind(&TcpConnection::handleClose,this));
}

TcpConnection::~TcpConnection()
{
    //loop_->RemoveChannelToPoller(ChannelPtr.get());
    close(fd_);
}

void TcpConnection::Shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!iswriting&&state_ == kDisconnecting)//此时没写了，直接调用close,否则在写，并且写完时关闭
    {                                          //这里必须重新判断state，随时可能在另一端关闭了
        shutdown(fd_,SHUT_WR);
        struct sockaddr_in clientaddr=shared_from_this()->GetclientAddress();
        //std::cout << "client from IP:" << inet_ntoa(clientaddr.sin_addr) 
           // << ":" << ntohs(clientaddr.sin_port) <<"is down"<< std::endl;
    }
}

void TcpConnection::connectEstablished(Timestamp receivetime)
{
    //loop的活动都加到runinloop，因为可能在其他线程
    //loop_->runInLoop(std::bind(&EventLoop::AddChannelToPoller,loop_,ChannelPtr.get()));
    loop_->AddChannelToPoller(ChannelPtr.get());
    assert(state_ == kConnecting);
    setState(kConnected);
    connectionCallback_(shared_from_this(),receivetime);
}

void TcpConnection::connectDestroyed()//不用这个了，两个线程来回调用，费时间并且出错
{
    assert(state_ == kConnected||state_ == kDisconnecting);
    setState(kDisconnected);
    //loop_->runInLoop(std::bind(&EventLoop::RemoveChannelToPoller,loop_,ChannelPtr.get()));
    loop_->RemoveChannelToPoller(ChannelPtr.get());
}


void TcpConnection::handleRead(Timestamp receivetime)
{
    int n=readn(fd_,bufferin_);
    if(n>0){
        messageCallback_(shared_from_this(), bufferin_,receivetime);
        bufferin_.clear();
    }
    else if(n==0)//收到FIN，shutdown关闭写端
        handleClose();
    else
        handleError();//可能收到RST，也要关闭

}

void TcpConnection::handleWrite()
{
    int n=sendn(fd_, bufferout_);
    if(n > 0)
    {
        uint32_t events = ChannelPtr->GetEvents();
        if(bufferout_.size() > 0)
        {
            //缓冲区满了，数据没发完，就设置EPOLLOUT事件触发			
            ChannelPtr->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(ChannelPtr.get());
        }
        else
        {
            //数据已发完
            ChannelPtr->SetEvents(events & (~EPOLLOUT));
            loop_->UpdateChannelToPoller(ChannelPtr.get());
            //sendcompletecallback_(shared_from_this());
            iswriting=false;
            //关闭写端
            if(state_==kDisconnecting&&readevent==0)//所有写事件都完成，可shutdown
            {
                //handleClose();
                shutdownInLoop();
            }

        }
    }
    else if(n < 0)
    {        
        handleError();
    }
    else
    {
        //handleClose();
    }  
}


void TcpConnection::Send(const std::string& buffer)//主动发送数据
{
    readevent++;//写事件加一
    int l1=bufferout_.size();
    bufferout_ += buffer; 
    int l2=bufferout_.size();
    iswriting=true;
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread()) {
            sendInLoop();
        }
        else
        {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, shared_from_this()));//直接加到队列里，自己判断
        }
        /*if(l1==0&&l2>0)//buffer变化才添加事件
        {
            uint32_t events = ChannelPtr->GetEvents();
            ChannelPtr->SetEvents(events | EPOLLOUT);
            loop_->runInLoop(std::bind(&EventLoop::UpdateChannelToPoller,loop_,ChannelPtr.get()));
        }*/
        
    }
}

void TcpConnection::sendInLoop()
{
    readevent--;//写事件减一，可能直接发完，也可能发不完，但都要减1
    int result = sendn(fd_, bufferout_);
    if(result > 0)
    {
		uint32_t events = ChannelPtr->GetEvents();
        if(bufferout_.size() > 0)
        {
			//缓冲区满了，数据没发完，就设置EPOLLOUT事件触发			
			ChannelPtr->SetEvents(events | EPOLLOUT);
			loop_->UpdateChannelToPoller(ChannelPtr.get());
        }
		else
		{
			//数据已发完
			ChannelPtr->SetEvents(events & (~EPOLLOUT));
            loop_->UpdateChannelToPoller(ChannelPtr.get());
			//sendcompletecallback_(shared_from_this());
            iswriting=false;
			if(state_==kDisconnecting&&readevent==0)//所有写事件都完成，可shutdown
            {
                shutdownInLoop();
                //handleClose();
            }

		}
    }
    else if(result < 0)
    {        
		handleError();
    }
	else
	{
		//handleClose();
	}     
}


void TcpConnection::handleClose()
{
    assert(state_ == kConnected||state_ == kDisconnecting);
    setState(kDisconnected);
    struct sockaddr_in clientaddr=shared_from_this()->GetclientAddress();
    //std::cout << "client from IP:" << inet_ntoa(clientaddr.sin_addr) 
        //<< ":" << ntohs(clientaddr.sin_port) <<"closed"<< std::endl;
    loop_->RemoveChannelToPoller(ChannelPtr.get());
    //ChannelPtr->SetEvents(0);
    //loop_->UpdateChannelToPoller(ChannelPtr.get());//先更新channel，将关注事件取消
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{   
    struct sockaddr_in clientaddr=shared_from_this()->GetclientAddress();
    std::cout << "client from IP:" << inet_ntoa(clientaddr.sin_addr) 
        << ":" << ntohs(clientaddr.sin_port) <<"error"<< std::endl;
    setState(kDisconnected);
    loop_->RemoveChannelToPoller(ChannelPtr.get());
    closeCallback_(shared_from_this());    
}





int TcpConnection::readn(int fd,std::string& bufferin)//ET，读到EAGAIN为止，即缓冲区没有可读
{
    int nbyte;
    int readsun=0;
    char buffer[BUFFSIZE];
    while(true)
    {
        nbyte=read(fd,buffer,BUFFSIZE);
        if(nbyte>0)
        {
            bufferin.append(buffer,nbyte);
            readsun+=nbyte;
            if(nbyte<BUFFSIZE)
                return readsun;//返回正数，读取正常
            else
                continue;
        }
        else if(nbyte<0)
        {
            if(errno==EAGAIN)//缓冲区读完，无数据
                return readsun;
            else if(errno==EINTR)
            {
                //调用非阻塞read，即使read被信号打断，read会会继续执行未完成的任务而不会去响应信号。
                //因为在非阻塞调用中，没有任何理由阻止read或者wirte的执行。
                continue;
            }
            else{
                //可能是RST，关闭
                perror("read error");
                return -1;
            }
        }
        else//read=0,可负担发送Fin
        {
            return 0;//先shutdown写端，缓冲区发完后close
        }
    }
}

int TcpConnection::sendn(int fd, std::string &bufferout)
{
	ssize_t nbyte = 0;
    int sendsum = 0;
	//char buffer[BUFSIZE+1];
	size_t length = 0;
	//无拷贝优化
	length = bufferout.size();
	if(length >= BUFFSIZE)
	{
		length = BUFFSIZE;
	}
	while(true)
	{
		nbyte = write(fd, bufferout.c_str(), length);
		if (nbyte > 0)
		{
            sendsum += nbyte;
			bufferout.erase(0, nbyte);
			//length = bufferout.copy(buffer, BUFSIZE, 0);
			//buffer[length] = '\0';
			length = bufferout.size();
			if(length >= BUFFSIZE)
			{
				length = BUFFSIZE;
			}
			if (length == 0 )
			{
				return sendsum;
			}
		}
		else if (nbyte < 0)//异常
		{
			if (errno == EAGAIN)//系统缓冲区满，非阻塞返回
			{
				return sendsum;
			}
			else if (errno == EINTR)
			{
				std::cout << "write errno == EINTR" << std::endl;
				continue;
			}
			else if (errno == EPIPE)
			{
				//客户端已经close，并发了RST，继续wirte会报EPIPE，返回-1，表示close
				perror("write error");
				return -1;
			}
			else
			{
				perror("write error");//Connection reset by peer
				return -1;
			}
		}
		else//返回0
		{
			//应该不会返回0
			//std::cout << "client close the Socket!" << std::endl;
			return 0;
		}
	}
}