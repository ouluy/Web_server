#include "threadpool.h"
#include "log.h"


pthread_mutex_t ThreadPool::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::notify = PTHREAD_COND_INITIALIZER;
std::vector<pthread_t> ThreadPool::threads;
std::vector<ThreadPoolTask> ThreadPool::queue;
int ThreadPool::thread_count = 0;
int ThreadPool::queue_size = 0;
int ThreadPool::head = 0;
int ThreadPool::tail = 0;
int ThreadPool::count = 0;
int ThreadPool::shutdown = 0;
int ThreadPool::started = 0;

int ThreadPool::ThreadpoolCreate(int _thread_count, int _queue_size)
{
    bool err = false;
    do
    {
        if(_thread_count <= 0 || _thread_count > MAX_THREADS || _queue_size <= 0 || _queue_size > MAX_QUEUE) 
        {
            _thread_count = 4;
            _queue_size = 1024;
        }
    
        thread_count = 0;
        queue_size = _queue_size;
        head = tail = count = 0;
        shutdown = started = 0;

        threads.resize(_thread_count);
        queue.resize(_queue_size);
    
        /* Start worker threads */
        for(int i = 0; i < _thread_count; ++i) 
        {
            if(pthread_create(&threads[i], NULL, ThreadpoolThread, (void*)(0)) != 0) 
            {
                //threadpool_destroy(pool, 0);
                return -1;
            }
            ++thread_count;
            ++started;
        }
    } while(false);
    
    if (err) 
    {
        //threadpool_free(pool);
        return -1;
    }
    printf("create: %d\n",queue_size);
    return 0;
}

void MyHandler(std::shared_ptr<void> req)
{
    std::shared_ptr<RequestData> request = std::static_pointer_cast<RequestData>(req);
    request->HandleRequest();
}

int ThreadPool::ThreadpoolAdd(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun)
{
    //printf("hello threadpool_add %d\n",queue_size);
    int next, err = 0;
    if(pthread_mutex_lock(&lock) != 0){
        LOG_ERROR("THREADPOOL_LOCK_FAILURE");
        return THREADPOOL_LOCK_FAILURE;
    }
    do {
        next = (tail + 1) % queue_size;
        // 队列满
       // printf("count:%d\n",count);
        if(count == queue_size) 
        {
            LOG_ERROR("THREADPOOL_QUEUE_FULL");
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        // 已关闭
        if(shutdown)
        {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        queue[tail].fun = fun;
        queue[tail].args = args;
        tail = next;
        ++count;
        //printf("fun:%d\n",fun);
        //printf("args:%d\n",args);
     //   printf("count2:%d\n",count);
        /* pthread_cond_broadcast */
        if(pthread_cond_signal(&notify) != 0) 
        {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while(false);

    if(pthread_mutex_unlock(&lock) != 0)
        err = THREADPOOL_LOCK_FAILURE;

    if(err==THREADPOOL_LOCK_FAILURE){
        LOG_ERROR("THREADPOOL_LOCK_FAILURE");
    }
    else if(err==THREADPOOL_SHUTDOWN){
        LOG_ERROR("THREADPOOL_SHUTDOWN");
    }
    return err;
}

int ThreadPool::ThreadpoolDestroy(threadpool_shutdown_t shutdown_option)
{
    printf("Thread pool destroy !\n");
    int i, err = 0;

    if(pthread_mutex_lock(&lock) != 0) 
    {
        return THREADPOOL_LOCK_FAILURE;
    }
    do 
    {
        if(shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        shutdown = shutdown_option;

        if((pthread_cond_broadcast(&notify) != 0) ||
           (pthread_mutex_unlock(&lock) != 0)) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        for(i = 0; i < thread_count; ++i)
        {
            if(pthread_join(threads[i], NULL) != 0)
            {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while(false);

    if(!err) 
    {
        ThreadpoolFree();
    }
    return err;
}

int ThreadPool::ThreadpoolFree()
{
    if(started > 0)
        return -1;
   // printf("xxxx\n");
    pthread_mutex_lock(&lock);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notify);
    return 0;
}

void *ThreadPool::ThreadpoolThread(void *args)
{
    while (true)
    {
        ThreadPoolTask task;
        pthread_mutex_lock(&lock);
        while((count == 0) && (!shutdown)) 
        {
            pthread_cond_wait(&notify, &lock);
            //将线程放在条件变量的请求队列后，内部解锁
            //线程等待被pthread_cond_broadcast信号唤醒或者pthread_cond_signal信号唤醒，唤醒后去竞争锁
            //若竞争到互斥锁，内部再次加锁
        }
        if((shutdown == immediate_shutdown) ||
           ((shutdown == graceful_shutdown) && (count == 0)))
        {
            break;
        }
        task.fun = queue[head].fun;
        task.args = queue[head].args;
        queue[head].fun = NULL;
        queue[head].args.reset();
        head = (head + 1) % queue_size;
        --count;
        pthread_mutex_unlock(&lock);
        task.fun(task.args);//function
    }

    --started;

    pthread_mutex_unlock(&lock);//上面break之后，缺一个解锁
    pthread_exit(NULL);
    return(NULL);
}