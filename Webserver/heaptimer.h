#ifndef HEAPTIMER
#define HEAPTIMER

#include"requestdata.h"
#include<string>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>

struct RequestData;

struct MyTimer{
private:
    static pthread_mutex_t lock;
    bool deleted;
    size_t expired_time;
    std::shared_ptr<RequestData> request_data;
public:
    MyTimer(std::shared_ptr<RequestData> _request_data, int timeout);
    ~MyTimer();
    void Update(int timeout);
    bool Isvalid();
    void ClearReq();
    void SetDeleted();
    bool IsDeleted() const;
    size_t GetExpTime() const;
};

struct TimerCmp{
    bool operator()(std::shared_ptr<MyTimer> &a,std::shared_ptr<MyTimer> &b) const;
};

class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

//private:
    pthread_mutex_t m_mutex;
};

#endif