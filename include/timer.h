#ifndef _TIME_HEAP_H_
#define _TIME_HEAP_H_

#include <iostream>
#include <netinet/in.h>
#include <time.h>

using std::exception;

#define BUFFER_SIZE 64

class HeapTimer;

struct ClientData
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    HeapTimer* timer;
}

class HeapTimer
{
public:
    HeapTimer(int delay);
    
public:
    time_t expire;
    void (*cb_func)(client_data*);
    client_data* user_data;
};

class TimeHeap
{
public:
    TimeHeap(int cap);
    TimeHeap(HeapTimer** init_array, int size, int capacity);
    ~TimeHeap();

public:
    void add_timer(HeapTimer* timer);
    void del_timer(HeapTimer* timer);
    HeapTimer* top() const { return empty() ? NULL : array[0];}
    void pop_timer();
    void tick();
    bool empty() const { return cur_size == 0;}

private:
    void percolate_down(int hole);
    void resize();

    HeapTimer** array;
    int capacity;
    int cur_size;
};

#endif
