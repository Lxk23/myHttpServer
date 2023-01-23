#ifndef _UTIL_H_
#define _UTIL_H_
//工具类

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

class Util
{
public:
    static int ReaLine(int sock, std::string& out);

    static void Cutstring(std::string target, std::string& out1, std::string& out2, std::string symbol);

    static pid_t GetThreadId();
    static uint32_t GetFiberId();

};

#endif
