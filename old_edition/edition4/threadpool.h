#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include"requestdata.h"
#include<pthread.h>
#include <functional>
#include <memory>
#include <vector>

const int THREADPOOL_INVALID = -1;        //线程池无效
const int THREADPOOL_LOCK_FAILURE = -2;   //线程池锁定失败
const int THREADPOOL_QUEUE_FULL = -3;     //线程池队列已满
const int THREADPOOL_SHUTDOWN = -4;       //线程池关闭
const int THREADPOOL_THREAD_FAILURE = -5; //线程池线程失败
const int THREADPOOL_GRACEFUL = 1;        //线程池优雅关闭

const int MAX_THREADS = 1024;           //最大线程数
const int MAX_QUEUE = 65535;            //最大队列数

typedef enum {
    immediate_shutdown = 1, //立即关闭
    graceful_shutdown  = 2  //自动关闭
} threadpool_shutdown_t;

struct ThreadPoolTask
{
    std::function<void(std::shared_ptr<void>)> fun;
    std::shared_ptr<void> args;
};

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
void MyHandler(std::shared_ptr<void> req);

class ThreadPool
{
private:
    static pthread_mutex_t lock;
    static pthread_cond_t notify;
    static std::vector<pthread_t> threads;
    static std::vector<ThreadPoolTask> queue;
    static int thread_count;
    static int queue_size;
    static int head;
    // tail 指向尾节点的下一节点
    static int tail;
    static int count;
    static int shutdown;
    static int started;
public:
    static int ThreadpoolCreate(int _thread_count, int _queue_size);
    static int ThreadpoolAdd(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun);
    static int ThreadpoolDestroy(threadpool_shutdown_t shutdown_option = graceful_shutdown);
    static int ThreadpoolFree();
    static void *ThreadpoolThread(void *args);
};


#endif