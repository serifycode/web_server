#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <sstream>
#include <map>

typedef struct _HttpRequestContext {
	std::string method;
	std::string url;
	std::string version;
	std::map<std::string, std::string> header;
	std::string body;
}HttpRequestContext;

typedef struct _HttpResponseContext {
    std::string version;
    std::string statecode;
    std::string statemsg;
	std::map<std::string, std::string> header;
	std::string body;
}HttpResponseContext;

class Http
{
private:
    //解析报文相关成员
    HttpRequestContext httprequestcontext;
    bool parseresult_;
    //长连接标志
    bool keepalive_;

public:

    Http();
    ~Http();

    //解析HTTP报文
    bool ParseHttpRequest(const std::string &msg); 

    //处理报文
    void HttpProcess(std::string &responsecontext); 

    //错误消息报文组装，404等
    void HttpError(const int err_num, const std::string short_msg, std::string &responsecontext);
    
    //判断长连接
    bool KeepAlive() 
    { return keepalive_;}
};

#endif