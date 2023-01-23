#include <stddef.h>
#include "protocal.h"

static std::string ContentTypeTable(std::string suffix)
{
    std::unordered_map<std::string,std::string> table{
        {".html","text/html"}, {".txt","text/plain"},
            {".ppt","application/vnd.ms-powerpoint"}, {".js","application/x-javascript"},
            {".xml","application/rss+xml"}
    };

    auto iter = table.find(suffix);
    if(iter != table.end())
    {
        return iter->second;
    }
    return ".html";

}

static std::string CodeAnaly(int code)
{
    std::string description;
    switch(code)
    {
        case 200:
            description = "OK";
            break;
        case 400:
            description = "BADREQUEST";
            break;
        case 404:
            description = "NOTFOUND";
            break;
        case 500:
            description = "SERVERERROR";
            break;
        default:
            break;
    }
    return description;
}

/*************HttpRequest******************/
HttpRequest::HttpRequest() : m_content_length(0), m_cgi(false) {
}

HttpRequest::~HttpRequest() {
}
/*************HttpRequest******************/

/*************HttpResponse*****************/
HttpResponse::HttpResponse() : m_status_code(OK) {
}

HttpResponse::~HttpResponse() {
}
/*************HttpResponse*****************/


/***************EndPoint*******************/
//读取请求
bool EndPoint::RecvHttpRequestLine()  //读取request请求行
{
    auto request_line = m_http_request.getRequestLine();
    int sz = Util::ReaLine(m_sock, request_line);
    m_http_request.setRequestLine(request_line); 

    if(sz>0)
    {
        request_line.resize(sz - 1);
        m_http_request.setRequestLine(request_line); 
        LOG_INFO(LOG_ROOT()) << m_http_request.getRequestLine(); 
    }
    else{
        m_stop = true;
    }
    return m_stop;
}

bool EndPoint::RecvHttpRequestHeader()//读取request报头和空行
{
    std::string s;
    while(true)
    {
        s.clear();
        if(Util::ReaLine(m_sock, s) <= 0)
        {
            m_stop = true;
            break;
        }
        if(s == "\n")
        {
            m_http_request.getBlank() = s;
            break;
        }
        s.resize(s.size() - 1);
        m_http_request.getRequestHeader().push_back(s);
    }
    return m_stop;
}


//解析请求
void EndPoint::ParseHttpRequestLine()//解析请求行
{
    auto line = m_http_request.getRequestLine();
    std::stringstream ss(line);
    std::string _method, _url, _version;
    ss >> _method >> _url >> _version;
    m_http_request.setMethod(_method);
    m_http_request.setUrl(_url);
    m_http_request.setVersion(_version);
    m_http_request.transformMethod();
}


void EndPoint::ParseURL()//解析URL
{
    auto method = m_http_request.getMethod();

    if(method != "GET" && method != "POST")  //如果method错误，那么我们不需要进行拆
    {
        m_http_response.setStatusCode(BAD_REQUEST);
        LOG_WARN(LOG_ROOT()) << "method error";
    }
    else if(method == "GET")//如果是GET方法，那么我们需要判断URI中是否带参数
    {
        auto url = m_http_request.getUrl();
        auto iter = url.find('?');

        if(iter != std::string::npos)
        {
            std::string _path;
            std::string _parameter;
            Util::Cutstring(url, _path, _parameter, "?");
            m_http_request.setPath(_path);
            m_http_request.setParameter(_parameter);
        }
        else 
        {
            m_http_request.setPath(url);
        } 
    }
    else if(method == "POST")//如果是POST方法,那么URI中只包含路径
    {
        m_http_request.setPath(m_http_request.getUrl());
        m_http_request.setCgi(true);
    }
}

void EndPoint::ParseHttpRequestHeader()//解析报头
{
    for(auto str : m_http_request.getRequestHeader())
    {
        std::string str1, str2;
        Util::Cutstring(str, str1, str2, SEP);
        m_http_request.getHeaderKv().insert(make_pair(str1, str2));
    }
}

bool EndPoint::IsNeedRecvHttpRequestBody()//判断是否需要取request正文
{
    auto method = m_http_request.getMethod();
    if(method == "POST")
    {
        //POST方法 有可能有正文
        auto head_kv = m_http_request.getHeaderKv();
        auto iter = head_kv.find("Content-Length");
        if(iter != head_kv.end())
        {
            //找到了
            m_http_request.setContentLength(atoi(iter->second.c_str()));
            return true;
        }        
    }
    return false;
}

void EndPoint::RecvHttpRequestBody()//读取正文
{
    if(IsNeedRecvHttpRequestBody())
    {
        auto content_length = m_http_request.getContentLength();
        char ch;
        while(content_length)
        {
            int sz = recv(m_sock, &ch, 1, 0);
            if(sz > 0)
            {
                //读取成功
                m_http_request.addToRequestBody(ch);
                content_length--;
            }
            else{
                m_stop = true;
                //读取失败
                break;
            }
        }
    }
}

int EndPoint::CgiProcess()
{
    int code = OK;
    auto method = m_http_request.getMethod();
    auto body = m_http_request.getRequestBody();
    auto parameter = m_http_request.getParameter();
    auto path = m_http_request.getPath();

    pid_t pid;
    int input[2];//对于父进程来说，是进行输入
    int output[2];//对于父进程来说，是进行输出

    if(pipe(input) < 0)
    {
        return SERVER_ERROR;
    }
    if(pipe(output) < 0)
    {
        return SERVER_ERROR;
    }

    pid = fork();
    if(pid == 0)
    {
        //子进程
        close(input[1]);
        close(output[0]);
        dup2(input[0], 0);
        dup2(output[1], 1);
        //环境变量不会被env给替换掉
        //将方法导入到子进程的环境变量中，让替换的子进程能够判断数据从哪里拿上来
        std::string method_env = "METHOD=";
        method_env += method;
        putenv((char*)method_env.c_str());

        if(method == "GET")
        {
            //如果是GET方法，GET方法传输的数据比较短，可以通过环境变量传输给子进程
            std::string parameter_env = "PARAMETER=";
            parameter_env += parameter;
            putenv((char*)parameter_env.c_str());
        }
        else if(method == "POST"){
            std::string length_env = "Content-Length=";
            length_env += std::to_string(m_http_request.getContentLength());
            std::cerr << length_env << std::endl;
            putenv((char*)length_env.c_str());
        }

        execl(path.c_str(), path.c_str(), nullptr);//execl会替换掉所有的数据和代码
        std::cout << "execl error" << std::endl;
        exit(1);
    }
    else if(pid > 0)
    {
        //父进程
        close(input[0]);
        close(output[1]);
        int sum = 0;
        int size = 0;
        auto start = body.c_str();
        if(method == "POST")//如果是POST方法，通过管道将数据输入给子进程
        {
            while((size = write(input[1], start + sum, body.size() - sum)) > 0)
            {
                sum += size; 
            }
        }

        char ch;
        while(read(output[0], &ch, 1) > 0)
        {
            m_http_response.addToResponseBody(ch);
        }
        std::cout << "body:" << m_http_response.getResponseBody() << std::endl;
        int status = 0;
        int ret = waitpid(pid, &status, 0);
        if(ret == pid)
        {
            if(WIFEXITED(status))
            {
                if(WIFSIGNALED(status) != 0)
                {
                    code = SERVER_ERROR;
                }
            }
            else{
                code = SERVER_ERROR;
            }
        }
        std::cout << "code" << code << std::endl;
        close(output[0]);
        close(input[1]);
    }
    else 
    {
        //创建失败
        code = 404;
    }
    return code;
}


int EndPoint::NotCgiProcess()
{
    auto _path = m_http_request.getPath();
    m_http_response.setFd(open(_path.c_str(), O_RDONLY));
    if(m_http_response.getFd() > 0)
    {         
        return OK;
    }
    return SERVER_ERROR; 
}


void EndPoint::RecvHttpRequest()
{
    //读取请求行和请求报头，如果请求行读取错误，设置相应的错误码
    if(!RecvHttpRequestLine() && !RecvHttpRequestHeader())
    {
        ParseHttpRequestLine();//解析请求行
        ParseURL();//解析URI
        ParseHttpRequestHeader();//解析报头
        RecvHttpRequestBody();//读取正文
    }
}

void EndPoint::BuildHttpResponse()//构建响应
{
    std::string path;
    auto code = m_http_response.getStatusCode();
    if(code == BAD_REQUEST)
    {
        goto END;
    }
    size_t pos;
    //在路径的最前面添加wwwroot/

    path = m_http_request.getPath();
    m_http_request.setPath(WWW_ROOT);
    m_http_request.addToPath(path); //wwwroot/a/b/c

    if(m_http_request.getPath()[m_http_request.getPath().size() - 1] == '/')
    {
        m_http_request.addToPath(HOME_PAGE); //wwwroot/index.html
    }
    std::cout << "path:" << m_http_request.getPath() << std::endl;

    struct stat st;
    if(stat(m_http_request.getPath().c_str(), &st) == 0)//判断该资源是否存在
    {
        if(S_ISDIR(st.st_mode))//判断是否为一个目录
        {
            //wwwroot/dir1 -> wwwroot/dir1/index.html
            //后缀都为.html
            m_http_request.addToPath("/");
            m_http_request.addToPath(HOME_PAGE);
            
            stat(m_http_request.getPath().c_str(), &st);
            m_http_response.setSuffix(".html");
        }
        //判断该资源是否为可执行文件
        else if(st.st_mode&S_IXGRP || st.st_mode&S_IXOTH || st.st_mode&S_IXUSR)
        {
            m_http_request.setCgi(true);//标识cgi处理
        }
        else{
            //do nothing
        }
    }
    else{
        //说明该文件不存在
        code = NOT_FOUND; //
        m_http_response.setStatusCode(NOT_FOUND);
        std::cout << "文件不存在:" << path << std::endl;
        goto END;
    }   

    //设置后缀
    m_http_response.setFileSize(st.st_size);

    pos = m_http_request.getPath().rfind(".");
    if(pos != std::string::npos)
    {
        //找到后缀
        m_http_response.setSuffix(m_http_request.getPath().substr(pos));
    }
    else{
        //找不到后缀,默认设置为.html
        m_http_response.setSuffix(".html");
    }
    if(m_http_request.hasCgi())
    {
        //进行cgi处理
        std::cout << "cgi begin..." << std::endl;
        code = CgiProcess();//将cgi处理完成的数据放在响应正文中
        m_http_response.setStatusCode(CgiProcess()); //
    }
    else{
        std::cout << "not cgi begining" << std::endl;
        code = NotCgiProcess();//打开文本文件，后续构建http响应
        m_http_response.setStatusCode(NotCgiProcess()); //
    }

END:
    BuildResponseHelper();
    return ;
}

void EndPoint::BuildResponseHelper()
{
    auto status_line = m_http_response.getStatusLine();
    auto code = m_http_response.getStatusCode();
        
    //构建响应行
    status_line = HTTP_VERSION;
    status_line += " ";
    status_line += std::to_string(code);
    status_line += " ";
    status_line += CodeAnaly(code);
    status_line += LINE_END;
    m_http_response.setStatusLine(status_line);
    m_http_response.setBlank(LINE_END);

    switch(code)
    {
        //根据不同的状态码来确定处理方式
        case 200:
            Build200header();
            break;
        case 400:
            BuildErrorHeader(PAGE_400);
            break;
        case 404:
            BuildErrorHeader(PAGE_404);
        case 500:
            BuildErrorHeader(PAGE_500);
            break;
        default:
            break;
    }
}

void EndPoint::BuildErrorHeader(std::string path)//构错误的响应报头
{      
    m_http_response.setFd(open(path.c_str(),O_RDONLY));
    if(m_http_response.getFd() > 0)//错误页面的文件被打开  
    {
        std::string line;
        //构建Content-Type,所有的错误页面都是文本文件，文件类型都为text/html
        line = "Content-Type: text/html";
        line += LINE_END;
        m_http_response.addToResponseHeader(line);
        //构建Content-Length,stat.st_size访问文本文件的大小
        line = "Content-Length: ";
        struct stat st;
        if(stat(path.c_str(), &st) == 0)
        {
            m_http_response.setFileSize(st.st_size);
        }
        line += std::to_string(m_http_response.getFileSize());
        line += LINE_END;
        m_http_response.addToResponseHeader(line);
    } 
}

void EndPoint::Build200header()//构建OK的响应报头
{
    auto cgi = m_http_request.hasCgi();
    auto response_header = m_http_response.getResponseHeader();    
    std::string line;
    //构建Content-Type,根据访问资源的后缀来判断是什么类型的文件
    //ContentThpeTable是一个根据资源返回相对应的类型
    line = "Content-Type: ";
    line += ContentTypeTable(m_http_response.getSuffix());
    line += LINE_END;
    //构建Content-Length
    response_header.push_back(line);
    line = "Content-Length: ";
    if(cgi)//如果是cgi处理，则处理的结果在响应正文中
    {
        line += std::to_string(m_http_response.getResponseBody().size());
    }
    else//如果是返回网页，则通过sendfile文件发送给浏览器，不需要读取到正中
    {
        line += std::to_string(m_http_response.getFileSize());
    }
    line += LINE_END;
    response_header.push_back(line);
}



void EndPoint::SendHttpResponse()
{
    auto cgi = m_http_request.hasCgi();
    send(m_sock, m_http_response.getStatusLine().c_str(), m_http_response.getStatusLine().size(), 0);
    for(auto str : m_http_response.getResponseHeader())
    {
        send(m_sock, str.c_str(), str.size(), 0);
    }
    send(m_sock, m_http_response.getBlank().c_str(), m_http_response.getBlank().size(), 0);
    if(cgi)     
    {
        //将cgi处理结果发送到网络中
        auto response_body = m_http_response.getResponseBody();
        int size = 0;
        size_t total = 0;
        const char* start = response_body.c_str();
        while(total < response_body.size() && (size = send(m_sock, start + total, response_body.size() - total, 0)) > 0)
        {
            total += size;
        }
    }
    else{
        //返回静态网页
        sendfile(m_sock, m_http_response.getFd(), 0, m_http_response.getFileSize());
        close(m_http_response.getFd());
    }
    LOG_INFO(LOG_ROOT()) << "SendHttpRespone";
}


/***************EndPoint*******************/


/***************HandlerTask*****************/

void HandlerTask::HandlerRequest(int sock)
{
    EndPoint* ep = new EndPoint(sock);
    ep->RecvHttpRequest();
    if(!ep->Stop())
    {
        ep->BuildHttpResponse();
        ep->SendHttpResponse();
    }
    close(sock);
    delete ep;
}
/***************HandlerTask*****************/
