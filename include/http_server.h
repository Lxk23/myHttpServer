#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <signal.h>
#include <memory>
#include "protocal.h"
#include "tcp_server.h"
#include "thread_pool.h"

#define PORT 8081

class HttpServer
{
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(int port=PORT);

    void InitServer();
    void Loop();

    ~HttpServer();

private:
    int m_port;
    TcpServer* m_tcp_server;
    ThreadPool* m_thread_pool;

    int idlefd;
};

#endif
