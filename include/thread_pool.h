#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <pthread.h>
#include <queue>
#include <memory>
#include "task.h"
#include "log.h"

#define NUM 3 

class ThreadPool
{
public:
    typedef std::shared_ptr<ThreadPool> ptr;

    static ThreadPool* GetThreadPool();

    void InitThreadPool();
    void PushTask(Task& in);
    
    ~ThreadPool();

private:
    int m_num;
    std::queue<Task> m_q;
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
    static ThreadPool* m_sigle_instance;

private:
    ThreadPool(int _num=NUM);

    static void* ThreadRoutine(void* arg);

    inline size_t Size() const { return m_q.size();}

    void Wait();
    void SignalThread();
    void PopTask(Task& out);
    void Lock();
    void UnLock();

};


#endif
