#ifndef _PROTOCAL_H_
#define _PROTOCAL_H_

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <unordered_map>
#include <sstream>
#include "util.h"
#include "log.h"

#define SEP ": "
#define OK 200
#define BAD_REQUEST 400 
#define NOT_FOUND 404
#define SERVER_ERROR 500
#define WWW_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.1"
#define PAGE_404 "wwwroot/404.html"
#define PAGE_400 "wwwroot/400.html"
#define PAGE_500 "wwwroot/500.html"
#define LINE_END "\r\n"

//将相应的后缀转换成相应的文件类型。
//如：./html -> text/html
static std::string ContentTypeTable(std::string suffix);

static std::string CodeAnaly(int code);


class HttpRequest
{
public:
    HttpRequest();
   ~HttpRequest();

    inline std::string getRequestLine() const { return m_request_line;}
    inline void setRequestLine(const std::string val) { m_request_line = val;}

    inline std::vector<std::string> getRequestHeader() const { return m_request_header;}
    inline std::string getBlank() const { return m_blank;}
    inline std::string getRequestBody() const { return m_request_body;}
    inline void addToRequestBody(char ch) { m_request_body.push_back(ch);}

    inline std::string getMethod() const { return m_method;}
    inline void setMethod(const std::string& val) { m_method = val;}
    inline void transformMethod() {
        std::transform(m_method.begin(), m_method.end(), m_method.begin(), ::toupper);
    }

    inline std::string getUrl() const { return m_url;}
    inline void setUrl(const std::string& val) { m_url = val;}

    inline std::string getVersion() const { return m_version;}
    inline void setVersion(const std::string& val) { m_version = val;}

    inline std::unordered_map<std::string, std:: string> getHeaderKv() const { return m_header_kv;}
    inline size_t getContentLength() const { return m_content_length;}
    inline void setContentLength(size_t val) { m_content_length = val;}

    inline std::string getPath() const { return m_path;}
    inline void setPath(const std::string& val) { m_path = val;}
    inline void addToPath(const std::string& val) { m_path += val;}

    inline std::string getParameter() const { return m_parameter;}
    inline void setParameter(const std::string& val) { m_parameter = val;}

    inline bool hasCgi() const { return m_cgi;}
    inline void setCgi(bool val) { m_cgi = val;}

private:
    std::string m_request_line;//请求行
    std::vector<std::string> m_request_header;//请求报头
    std::string m_blank;//请求空行
    std::string m_request_body;//请求正文

    //请求行的解析
    std::string m_method;
    std::string m_url;
    std::string m_version;

    //请求报头的解析
    std::unordered_map<std::string, std::string> m_header_kv;
    size_t m_content_length;//请求正文的大小

    //请求行的解析
    std::string m_path;//请求路径
    std::string m_parameter;//参数

    bool m_cgi;//CGI处理的标识符
};

class HttpResponse
{
public:
     HttpResponse();
    ~HttpResponse();

    inline std::string getStatusLine() const { return m_status_line;}
    inline void setStatusLine(const std::string& val) { m_status_line = val;}

    inline std::vector<std::string> getResponseHeader() const { return m_response_header;}
    inline void addToResponseHeader(const std::string& val) { m_response_header.push_back(val);}
    inline std::string getBlank() const { return m_blank;}
    inline void setBlank(const std::string& val) { m_blank = val;}

    inline std::string getResponseBody() const { return m_response_body;}
    inline void addToResponseBody(char ch) { m_response_body.push_back(ch);}

    inline int getStatusCode() const { return m_status_code;}
    inline void setStatusCode(int val) { m_status_code = val;}

    inline int getFd() const { return m_fd;}
    inline void setFd(int val) { m_fd = val;}

    inline int getFileSize() const { return m_file_size;}
    inline void setFileSize(int val) { m_file_size = val;}

    inline std::string getSuffix() const { return m_suffix;}
    inline void setSuffix(const std::string& val) { m_suffix = val;}
    
private:
    std::string m_status_line;//状态行
    std::vector<std::string> m_response_header;//响应报头
    std::string m_blank;//响应空行
    std::string m_response_body;//响应正文

    int m_status_code;//响应的状态码
    int m_fd;//打开的文件描述符
    int m_file_size;//打开的文件大小
    std::string m_suffix;//访问资源的后缀
};

class EndPoint
{
public:
    EndPoint(int sock) : m_sock(sock) {
    }
    ~EndPoint() {
    }

    inline bool Stop() const { return m_stop;}
    void RecvHttpRequest();
    void BuildHttpResponse(); //构建响应
    void BuildResponseHelper();
    void BuildErrorHeader(std::string path); //构错误的响应报头
    void Build200header();//构建OK的响应报头
    void SendHttpResponse();

private:
    int m_sock;
    HttpRequest m_http_request;
    HttpResponse m_http_response;
    bool m_stop;

private:
    EndPoint() : m_stop(false) {
    } 
    
    //读取请求
    bool RecvHttpRequestLine(); //读取request请求行
    bool RecvHttpRequestHeader(); //读取request报头和空行
    //解析请求
    void ParseHttpRequestLine();//解析请求行
    void ParseURL(); //解析URL
    void ParseHttpRequestHeader(); //解析报头
    bool IsNeedRecvHttpRequestBody(); //判断是否需要取request正文
    void RecvHttpRequestBody(); //读取正文
    int CgiProcess();
    int NotCgiProcess();

};

class HandlerTask
{
public:
    inline void operator()(int sock)
    {
      LOG_INFO(LOG_ROOT()) << "HandlerTask";
      HandlerRequest(sock);
    }

private:
    void HandlerRequest(int sock);
};

#endif
