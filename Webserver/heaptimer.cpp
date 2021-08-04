#include "requestdata.h"
#include "wrap.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>
#include <memory>
#include "heaptimer.h"
#include <iostream>
using namespace std;


size_t Calculation(long a,long b,int c){
    return (a*1000+b/1000)+c;// c/1000 s
}

pthread_mutex_t MyTimer::lock = PTHREAD_MUTEX_INITIALIZER;

MyTimer::MyTimer(shared_ptr<RequestData> _request_data, int timeout): 
    deleted(false), request_data(_request_data)
{
    //cout << "mytimer()" << endl;
    struct timeval now;
    /*
    struct timeval {
        long  tv_sec;   秒数
        long  tv_usec;  微秒数 
    }
    */
    gettimeofday(&now, NULL);
    // 以毫秒计（1970年1月1日到现在的时间）
    //1000000 微秒 = 1秒
    expired_time = Calculation(now.tv_sec,now.tv_usec,timeout);
   // cout<<expired_time<<endl;
}

MyTimer::~MyTimer()
{
    cout<<"requestdata:"<<request_data<<" ~TimerNode()"<<endl;
    if (request_data)
    {
        Epoll::EpollDel(request_data->GetFd(), EPOLLIN | EPOLLET | EPOLLONESHOT);
    }
}

void MyTimer::Update(int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    expired_time = Calculation(now.tv_sec,now.tv_usec,timeout);
}

bool MyTimer::Isvalid()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = ((now.tv_sec * 1000) + (now.tv_usec / 1000));
    if (temp < expired_time)
    {
        return true;
    }
    else
    {
        this->SetDeleted();
        return false;
    }
}

void MyTimer::ClearReq()
{
    request_data.reset();
    this->SetDeleted();
}

void MyTimer::SetDeleted()
{
    deleted = true;
}

bool MyTimer::IsDeleted() const
{
    return deleted;
}

size_t MyTimer::GetExpTime() const
{
    return expired_time;
}

bool TimerCmp::operator()(shared_ptr<MyTimer> &a, shared_ptr<MyTimer> &b) const
{
    return a->GetExpTime() > b->GetExpTime();
}
