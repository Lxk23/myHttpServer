#include <stdlib.h>
#include "tcp_server.h"


TcpServer* TcpServer::m_tcp_svr = nullptr;

TcpServer* TcpServer::GetTcpServer(int port)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    if(m_tcp_svr == nullptr)
    {
        pthread_mutex_lock(&lock); 
        if(m_tcp_svr == nullptr)
        {
            m_tcp_svr = new TcpServer(port);
            m_tcp_svr->InitServer();
        }
        pthread_mutex_unlock(&lock);

        return m_tcp_svr;
    }
    return m_tcp_svr;
}

void TcpServer::InitServer()
{
    Socket();
    Bind();
    Listen();
    LOG_INFO(LOG_ROOT()) << "InitServer success..";
}

void TcpServer::Socket()
{
    m_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(m_listen_sock < 0)
    {
        ERR_EXIT("socket");
    }
    LOG_INFO(LOG_ROOT()) << "socket success";
    int opt = 1;
    setsockopt(m_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void TcpServer::Bind()
{
    struct sockaddr_in local;
    memset(&local, '\0', sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(m_port);
    local.sin_addr.s_addr = INADDR_ANY;
    if(bind(m_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0)
    {
        ERR_EXIT("bind");
    }
    LOG_INFO(LOG_ROOT()) << "bind success";
}

void TcpServer::Listen()
{
    if(listen(m_listen_sock, BACKLOG) < 0)
    {
        ERR_EXIT("listen");
    }
    LOG_INFO(LOG_ROOT()) << "listen success";

}

