#include "Http.h"
#include <iostream>
#include <stdio.h>
#include <string.h>


Http::Http():keepalive_(false),
            parseresult_(false)
{

}

Http::~Http()
{

}

bool Http::ParseHttpRequest(const std::string& msg)
{
    std::string crlf("\r\n"), crlfcrlf("\r\n\r\n");
	size_t prev = 0, next = 0, pos_colon;
	std::string key, value;

    bool praseresult = false;
    //以下解析可以改成状态机，解决一次收Http报文不完整问题
	//prase http request line
	if ((next = msg.find(crlf, prev)) != std::string::npos)
	{
		std::string first_line(msg.substr(prev, next - prev));
		prev = next;
		std::stringstream sstream(first_line);
		sstream >> (httprequestcontext.method);
		sstream >> (httprequestcontext.url);
		sstream >> (httprequestcontext.version);
	}
	else
	{
        std::cout << "msg" << msg << std::endl;
		std::cout << "Error in httpPraser: http_request_line isn't complete!" << std::endl;
        praseresult = false;
        return praseresult;
        //可以临时存起来，凑齐了再解析
	}
    //prase http request header
	size_t pos_crlfcrlf = 0;
	if (( pos_crlfcrlf = msg.find(crlfcrlf, prev)) != std::string::npos)
	{
		while (prev != pos_crlfcrlf)
        {
            next = msg.find(crlf, prev + 2);
            pos_colon = msg.find(":", prev + 2);
            key = msg.substr(prev + 2, pos_colon - prev-2);
            value = msg.substr(pos_colon + 2, next-pos_colon-2);
            prev = next;
            httprequestcontext.header.insert(std::pair<std::string, std::string>(key, value));
        }
	}
    else
    {
        std::cout << "Error in httpPraser: http_request_header isn't complete!" << std::endl;
        praseresult = false;
        return praseresult;
    }	
    //prase http request body
	httprequestcontext.body = msg.substr(pos_crlfcrlf + 4);
    praseresult = true;
    return praseresult;
}


void Http::HttpProcess(std::string &responsecontext)
{
    std::string responsebody;
    std::string errormsg;
    std::string path;
    std::string querystring;
    
    if("GET" == httprequestcontext.method)
    {
        ;
    }
    else if("POST" == httprequestcontext.method)
    {
        ;
    }
    else
    {
        std::cout << "HttpServer::HttpParser" << std::endl;
        errormsg = "Method Not Implemented";
        HttpError(501, std::string("Method Not Implemented"), responsecontext);
        return;
    }

    size_t pos = httprequestcontext.url.find("?");
    if(pos != std::string::npos)
    {
        path = httprequestcontext.url.substr(0, pos);
        querystring = httprequestcontext.url.substr(pos+1);
    }
    else
    {
        path = httprequestcontext.url;
    }
    
    //keepalive判断处理
    std::map<std::string, std::string>::const_iterator iter = httprequestcontext.header.find("Connection");
    if(iter != httprequestcontext.header.end())
    {
        keepalive_ = (iter->second == "Keep-Alive");
    }
    else
    {
        if(httprequestcontext.version == "HTTP/1.1")
        {
            keepalive_ = true;//HTTP/1.1默认长连接
        }
        else
        {
            keepalive_ = false;//HTTP/1.0默认短连接
        }            
    }

    
    if("/" == path)
    {        
        path = "/index.html";
    }
    else if("/hello" == path)
    {
        //Wenbbench 测试用
        std::string filetype("text/html");
        responsebody = ("hello world");
        responsecontext += httprequestcontext.version + " 200 OK\r\n";
        responsecontext += "Server: Chen Shuaihao's NetServer/0.1\r\n";
        responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
        if(iter != httprequestcontext.header.end())
        {
            responsecontext += "Connection: " + iter->second + "\r\n";
        }
        responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
        responsecontext += "\r\n";
        responsecontext += responsebody;
        return;
    }
    else
    {
        ;
    }    

    //std::string responsebody;    
    path.insert(0,"/home/serify/serify_project/web");
    FILE* fp = NULL;
    if((fp = fopen(path.c_str(), "rb")) == NULL)
    {
        //perror("error fopen");
        //404 NOT FOUND
        HttpError(404, std::string("Not Found"), responsecontext);
        return;
    }
    else
    {
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        while(fread(buffer, sizeof(buffer), 1, fp) == 1)//可以mmap内存映射优化
        {
            responsebody.append(buffer);
            memset(buffer, 0, sizeof(buffer));
        }
        if(feof(fp))
        {
            responsebody.append(buffer);
        }        
        else
        {
            std::cout << "error fread" << std::endl;
        }        	
        fclose(fp);
    }

    std::string filetype("text/html"); //暂时固定为html
    responsecontext += httprequestcontext.version + " 200 OK\r\n";
    responsecontext += "Server: Chen Shuaihao's NetServer/0.1\r\n";
    responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
    if(iter != httprequestcontext.header.end())
    {
        responsecontext += "Connection: " + iter->second + "\r\n";
    }
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    responsecontext += "\r\n";
    responsecontext += responsebody;    
}

void Http::HttpError(const int err_num, const std::string short_msg,std::string &responsecontext)
{
    std::string responsebody;
    responsebody += "<html><title>出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += std::to_string(err_num) + " " + short_msg;
    responsebody += "</h1><hr><em> Chen Shuaihao's NetServer</em>\n</body></html>";

    std::string httpversion;
    if(httprequestcontext.version.empty())
    {
        httpversion = "HTTP/1.1";
    }
    else
    {
        httpversion = httprequestcontext.version;
    }   
        
    responsecontext += httpversion + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
    responsecontext += "Server: Chen Shuaihao's NetServer/0.1\r\n";
    responsecontext += "Content-Type: text/html\r\n";
    responsecontext += "Connection: Keep-Alive\r\n";
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    responsecontext += "\r\n";
    responsecontext += responsebody;
}