#ifndef _TASK_H_
#define _TASK_H_

#include "protocal.h"

class Task
{
public:
    Task() {
    }

    Task(int sock) : m_sock(sock) {
    }

    void HanderTask()
    {
      m_hander(m_sock);
    }

    ~Task() {
    }

private:
    int m_sock;
    HandlerTask m_hander;
};

#endif
