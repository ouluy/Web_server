#include "requestdata.h"
#include "epoll.h"
#include "threadpool.h"
#include "wrap.h"
#include <sys/epoll.h>
#include <queue>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <memory>
#include "heaptimer.h"
#include "log.h"


using namespace std;

static const int MAXEVENTS = 5000;
static const int LISTENQ = 1024;
const int THREADPOOL_THREAD_NUM = 4;
const int QUEUE_SIZE = 65535;

const int PORT = 8888;
const int ASK_STATIC_FILE = 1;
const int ASK_IMAGE_STITCH = 2;

const int LOG_LEVEL=1;
const int LOG_QUEUE_SIZE=1024;

const string PATH = "/";

void acceptConnection(int listen_fd, int epoll_fd, const string &path);

extern std::priority_queue<shared_ptr<MyTimer>, std::deque<shared_ptr<MyTimer>>, TimerCmp> myTimerQueue;


int socket_bind_listen(int port)
{
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        LOG_ERROR("Create socket error!", port);
        return -1;
    }

    // 消除bind时"Address already in use"错误
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)) == -1){
        LOG_ERROR("Init linger error!", port);
        return -1;
    }

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        LOG_ERROR("Bind Port:%d error!", port);
        return -1;
    }

    // 开始监听，最大等待队列长为LISTENQ
    if(listen(listen_fd, LISTENQ) == -1){
        LOG_ERROR("Listen port:%d error!", port);
        return -1;
    }

    // 无效监听描述符
    if(listen_fd == -1)
    {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}


int main()
{
    HandleForSigpipe();
    //忽略SIGPIPE信号，防止因为错误的写操作（向读端关闭的socket中写数据）而导致进程退出

    if (Epoll::EpollInit(MAXEVENTS, LISTENQ) < 0)
    {
        perror("epoll init failed");
        return 1;
    }
    if (ThreadPool::ThreadpoolCreate(THREADPOOL_THREAD_NUM, QUEUE_SIZE) < 0)
    {
        printf("Threadpool create failed\n");
        return 1;
    }  
    int ret = chdir("/home/ouluy/Desktop/cs/");
   // printf("%d\n",ret);
    if(ret == -1){
        perror("chdir error");
        return 1;
    }

    int listen_fd = socket_bind_listen(PORT);
  
   // printf("listen_fd:%d\n",listen_fd);

    if (listen_fd < 0) 
    {
        perror("socket bind failed");
        return 1;
    }
    if (SetSocketNonBlocking(listen_fd) < 0)
    //send是将信息发送给套接字缓冲区，如果缓冲区满了，则会阻塞
    //这时候会进一步增加信号处理函数的执行时间，为此，将其修改为非阻塞
    {
        LOG_INFO("Server port:%d", PORT);
        perror("set socket non block failed");
        return 1;
    }
    Log::get_instance()->init("./myserver", 0, 2000, 800000, LOG_QUEUE_SIZE);
    printf("yes\n");
    LOG_INFO("========== Server init ==========");
    printf("yes2\n");
    LOG_INFO("Port:%d", PORT);
    LOG_INFO("LogSys level: %d", LOG_LEVEL);

    shared_ptr<RequestData> request(new RequestData());

    request->SetFd(listen_fd);
    
    if (Epoll::EpollAdd(listen_fd, request, EPOLLIN | EPOLLET) < 0)
    {
        LOG_ERROR("Add listen error!");
        perror("epoll add error");
        return 1;
    }




    while (true)
    {
      //  sleep(10);
        Epoll::MyEpollWait(listen_fd, MAXEVENTS, -1);
    }
    return 0;
}