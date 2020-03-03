//对read和write返回值做各种判断，加入shutdown，加入多线程




#include "TcpServer.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include <iostream>
#include <string>
#include <unistd.h> 

int sleepSeconds = 5;
std::string message1;
std::string message2;
void onConnection(const TcpConnectionPtr& conn,Timestamp receivetime)
{
    if (conn->connected())
    {
        struct sockaddr_in clientaddr=conn->GetclientAddress();
        std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
            << ":" << ntohs(clientaddr.sin_port) <<" at "
            <<receivetime.toFormattedString() <<std::endl;
        conn->Send(message1);
        conn->Send(message2);
        //conn->Shutdown();
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
}

void Ontimer(Timestamp receivetime)
{
    std::cout<<"timer work"<<receivetime.toFormattedString()<<std::endl;
}


int main(int argc, char* argv[])
{
    std::cout<<"mainid:"<<std::this_thread::get_id()<<std::endl;

    int len1 = 1000;
    int len2 = 2000;

    if (argc > 2)
    {
        len1 = atoi(argv[1]);
        len2 = atoi(argv[2]);
    }

    message1.resize(len1);
    message2.resize(len2);
    std::fill(message1.begin(), message1.end(), 'A');
    std::fill(message2.begin(), message2.end(), 'B');


    EventLoop loop;

    TcpServer server(&loop, 80,0);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    loop.runevery(1,Ontimer);
    server.start();

    loop.loop();
}