#include "thread_pool.h"

ThreadPool* ThreadPool::m_sigle_instance = nullptr;

ThreadPool* ThreadPool::GetThreadPool()
{
    LOG_INFO(LOG_ROOT()) << "GetThreadPool";
    static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
    if(m_sigle_instance == nullptr)
    {
        pthread_mutex_lock(&_mutex);
        if(m_sigle_instance == nullptr)
        {
            m_sigle_instance = new ThreadPool();
            m_sigle_instance->InitThreadPool();
        }
        pthread_mutex_unlock(&_mutex);
    }
    return m_sigle_instance;
}

void ThreadPool::InitThreadPool()
{
    pthread_t tid;
    for(int i = 0; i < m_num; i++)
    {
        pthread_create(&tid, nullptr, ThreadRoutine, this);
    }
}

void ThreadPool::PushTask(Task& in)
{
    Lock();
    m_q.push(in);
    UnLock();
    if(m_q.size() > 0)
    {
        SignalThread();
    }
}

ThreadPool::~ThreadPool() {
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
} 

ThreadPool::ThreadPool(int num /*=NUM*/) : m_num(num){
    pthread_cond_init(&m_cond, nullptr);
    pthread_mutex_init(&m_mutex, nullptr);
}

void* ThreadPool::ThreadRoutine(void* arg)
{
    LOG_INFO(LOG_ROOT()) << "ThreadRoutine";
    pthread_detach(pthread_self());

    ThreadPool* thread=(ThreadPool*)arg;
    for( ; ; )
    {
        thread->Lock();
        while(thread->Size() == 0)
        {
            thread->Wait();
        }
        Task t;
        thread->PopTask(t);
        thread->UnLock();
        t.HanderTask();
    }
    return thread;
}

void ThreadPool::Wait()
{
    pthread_cond_wait(&m_cond, &m_mutex);
}

void ThreadPool::SignalThread()
{
    pthread_cond_signal(&m_cond);
}

void ThreadPool::PopTask(Task& out)
{
    out = m_q.front();
    m_q.pop();
}

void ThreadPool::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

void ThreadPool::UnLock()
{
    pthread_mutex_unlock(&m_mutex);
}

