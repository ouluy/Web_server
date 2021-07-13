#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include"requestdata.h"
#include<pthread.h>

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

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.(function 指向将执行任务的函数的指针)
 *  @var argument Argument to be passed to the function.(argument 要传递给函数的参数)
 */

typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.(条件变量通知工作线程)
 *  @var threads      Array containing worker threads ID.(数组包含工作线程ID)
 *  @var thread_count Number of threads(计数线程数)
 *  @var queue        Array containing the task queue.(数组包含任务队列)
 *  @var queue_size   Size of the task queue.(任务队列的大小)
 *  @var head         Index of the first element.(第一个元素的头索引)
 *  @var tail         Index of the next element.(下一个元素的尾部索引)
 *  @var count        Number of pending tasks(挂起任务数)
 *  @var shutdown     Flag indicating if the pool is shutting down(标志指示池是否正在关闭)
 *  @var started      Number of started threads(启动线程数)
 */
struct threadpool_t{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
};

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);
int threadpool_destroy(threadpool_t *pool, int flags);
int threadpool_free(threadpool_t *pool);
static void *threadpool_thread(void *threadpool);

#endif