# Webserver
Learn and make your own webserver

用于记录学习和写好一个Web服务器项目的过程

>server.c client.c wrap.c

实现了server和client连通和错误处理函数的封装

>multiprocess_server.c

实现了多进程并发服务器，避免僵尸进程

>multithreading_server.c

实现了多线程并发服务器，防止僵线程产生

* gcc multithreading_server.c wrap.c -o server -lpthread 

* 创建server.o的需要特别加上-lpthread

>select_server.c

select实现多路IO转接,添加自定义数组提高速率

>epoll_loop.c

epoll 反应堆模型：

epoll ET模式 + 非阻塞、轮询 + void *ptr

![epoll反应堆模型的流程](https://github.com/ouluy/Web_server/blob/main/example/Web_server/epoll.png)

>threadpool.c

threadpool 线程池 实现了为突然大量爆发的线程设计的，通过有限的几个固定线程为大量的操作服务，减少了创建和销毁线程所需的时间，从而提高效率
