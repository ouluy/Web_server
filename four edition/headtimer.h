#ifndef HEADTIMER
#define HEADTIMER

#include"requestdata.h"
#include<string>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>

struct requestData;

struct mytimer{
    bool deleted;
    size_t expired_time;
    std::shared_ptr<requestData> request_data;

    mytimer(std::shared_ptr<requestData> _request_data, int timeout);
    ~mytimer();
    void update(int timeout);
    bool isvalid();
    void clearReq();
    void setDeleted();
    bool isDeleted() const;
    size_t getExpTime() const;
};

struct timerCmp{
    bool operator()(std::shared_ptr<mytimer> &a,std::shared_ptr<mytimer> &b) const;
};

class MutexLockGuard
{
public:
    explicit MutexLockGuard();
    ~MutexLockGuard();

private:
    static pthread_mutex_t lock;

private:
    MutexLockGuard(const MutexLockGuard&);
    MutexLockGuard& operator=(const MutexLockGuard&);
};

#endif