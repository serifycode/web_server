//实现时间轮提出连接，ENTRY含有Tcpconnection弱指针，析构时会关闭连接

#include "Entry.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "Timestamp.h"

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
        std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
            << ":" << ntohs(clientaddr.sin_port) <<" at "
            <<receivetime.toFormattedString() <<std::endl;
        conn->Send(message1);
        conn->Send(message2);
    }
    else
    {
        std::cout << "New client from IP:" << std::endl;
    }

}

void onMessage(const TcpConnectionPtr& con,const std::string& ss,Timestamp receivetime)
{
    struct sockaddr_in clientaddr=con->GetclientAddress();
    std::cout <<"receive from IP"<<inet_ntoa(clientaddr.sin_addr) 
        << ":" << ntohs(clientaddr.sin_port)<<"messege:"
        <<ss.c_str()<<" at "<<receivetime.toFormattedString() <<std::endl;
    con->Send(ss);
    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(con->getContext()));
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

    int len1 = 1000;
    int len2 = 2000;
    message1.resize(len1);
    message2.resize(len2);
    std::fill(message1.begin(), message1.end(), 'A');
    std::fill(message2.begin(), message2.end(), 'B');

    EventLoop loop;
    TcpServer server(&loop, 80,4);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    loop.runevery(1,Ontimer);
    server.start();
    loop.loop();
}

