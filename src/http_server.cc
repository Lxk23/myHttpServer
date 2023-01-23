#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include "http_server.h"
//#include "Log.h"
#include "log.h"
#include "task.h"


HttpServer::HttpServer(int port /*=PORT*/) : m_port(port) {
    idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
}

void HttpServer::InitServer()
{
    //信号SIGPIPE需要进行忽略，如果不忽略，客户端关闭，会导致服务器挂掉
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);   //避免将死进程

    m_tcp_server = TcpServer::GetTcpServer(m_port);
    m_thread_pool = ThreadPool::GetThreadPool();
}

void HttpServer::Loop()
{
    int listen_sock = m_tcp_server->getListenSock();
    while(true)
    {
        struct sockaddr_in peer;
        socklen_t sz = sizeof(peer);
        int sock = accept4(listen_sock, (struct sockaddr*)&peer, &sz, SOCK_NONBLOCK | SOCK_CLOEXEC);
        LOG_INFO(LOG_ROOT()) << "accept success!";

        if(sock < 0)
        {
            //优雅地断开了与客户端的连接
            if(errno == EMFILE)
            {
                close(idlefd);
                idlefd = accept(listen_sock, NULL, NULL);
                close(idlefd);
                idlefd = open("dev/null", O_RDONLY | O_CLOEXEC);

                LOG_ERROR(LOG_ROOT()) << "the number of file descriptors reaches the upper limit, close the client connection";
                continue;
            }
            else {
                ERR_EXIT("accept4");
            }
        }
        Task t(sock);
        m_thread_pool->PushTask(t);
    } 
}

HttpServer::~HttpServer() {
}
