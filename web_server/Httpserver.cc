#include "Entry.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include "Http.h"

#include <iostream>
#include <string>
#include <unordered_set>
#include <deque>

typedef std::unordered_set<EntryPtr> Bucket;
typedef std::deque<Bucket> WeakConnectionList; 

WeakConnectionList connectionBuckets_;

std::string message1;
std::string message2;

void onConnection(const TcpConnectionPtr& conn,Timestamp receivetime)
{
    if (conn->connected())
    {
        EntryPtr entry(new Entry(conn));
        connectionBuckets_.back().insert(entry);
        WeakEntryPtr weakEntry(entry);
        conn->setContext(weakEntry);
        struct sockaddr_in clientaddr=conn->GetclientAddress();
        //std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
            //<< ":" << ntohs(clientaddr.sin_port) <<" at "
            //<<receivetime.toFormattedString() <<std::endl;
    }
    else
    {
        std::cout << "New client from IP:" << std::endl;
    }

}

void onMessage(const TcpConnectionPtr& con,const std::string& ss,Timestamp receivetime)
{
    struct sockaddr_in clientaddr=con->GetclientAddress();
    //std::cout <<"receive from IP"<<inet_ntoa(clientaddr.sin_addr) 
        //<< ":" << ntohs(clientaddr.sin_port)<<"messege:"
       //<<" at "<<receivetime.toFormattedString() <<std::endl;

    Http http_;
    std::string response_messege;
    if(http_.ParseHttpRequest(ss))//解析成功
    {
        http_.HttpProcess(response_messege);
    }
    else
    {
        http_.HttpError(400, "Bad request",response_messege);
    }    
    con->Send(response_messege);
    if(!http_.KeepAlive())//短连接直接关掉
        con->handleClose();

    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(con->getContext()));//全都加到其中，不影响短连接
    EntryPtr entry(weakEntry.lock());
    if (entry)
    {
        connectionBuckets_.back().insert(entry);
    }
}

void Ontimer(Timestamp receivetime)
{
    if(connectionBuckets_.size()>=5)
        connectionBuckets_.pop_front();
    connectionBuckets_.push_back(Bucket()); 
    //std::cout<<"timer work"<<receivetime.toFormattedString()<<std::endl;       
}

int main()
{
    std::cout<<"mainid:"<<std::this_thread::get_id()<<std::endl;

    EventLoop loop;
    TcpServer server(&loop, 80,4);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    loop.runevery(1,Ontimer);
    server.start();
    loop.loop();
}

