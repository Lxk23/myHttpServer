#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <iostream>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <memory>
#include "log.h"

#define PORT 8081
#define BACKLOG 5  

#define ERR_EXIT(m) \
    do \
    { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)


class TcpServer
{
public:
    typedef std::shared_ptr<TcpServer> ptr;

    static TcpServer* GetTcpServer(int port);
    inline int getListenSock() const { return m_listen_sock;}

    void InitServer();
    void Socket();
    void Bind();
    void Listen();

    ~TcpServer() {
    }

private:
    int m_port;
    int m_listen_sock;
    static TcpServer* m_tcp_svr;

private:
    TcpServer(int port) : m_port(port), m_listen_sock(-1) {
    }

};


#endif
