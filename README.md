# myHttpServer
A web server based on the HTTP 1.1 protocol

##介绍<br>
 本项目为c++11编写的web服务器，解析了get、post请求，可以处理静态资源，实现了CGI机制，支持HTTP长连接，并实现了异步日志，记录服务器运行状态。<br>
 
 ##环境<br>
 OS：Ubuntu16.04<br>
 编译器：cmake3.0及以上<br>
 
 ##编译<br>
 1、命令编译<br>
 mkdir build && cd build && cmake .. && make<br>
 2、直接使用shell脚本<br>
 ./build.sh<br>
 
 ##启动服务器<br>
 ./httpserver [port]<br>
 
 ##关键技术<br>
 *使用Epoll边沿触发的IO多路复用技术，非阻塞IO；<br>
 *使用Reactor模式，主线程只负责监听事件，工作线程通过轮询的方式去处理有事件发生的监听；<br>
 *支持长连接，对于一定时间不活跃的客户端，服务器添加主动断开连接机制，尽量避免在服务器端大量出现TIME_WAIT状态；<br>
 *使用互斥锁和条件变量实现线程同步，锁的竞争只会出现在主线程和某一特定工作线程之间；<br>
 *使用sendfile系统调用避免内核缓冲区和用户缓冲区之间的数据拷贝，实现零拷贝；<br>
 *实现了CGI机制，处理请求资源为可执行程序的情形；<br>
 *支持优雅关闭连接；<br>
 *充分利用c++11中的好功能，尤其为减少内存泄漏的可能性，尽可能多地使用智能指针；<br>
 *日志系统支持输出到控制台和日志文件中，记录服务器运行状态； <br>
 *使用测试工具Webbench对服务器进行压力测试；<br>
