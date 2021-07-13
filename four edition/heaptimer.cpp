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


/*#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
using namespace cv;
*/
//test
#include <iostream>
using namespace std;


size_t jisuan(long a,long b,int c){
    return (a*1000+b/1000)+c;// c/1000 s
}

pthread_mutex_t mytimer::lock = PTHREAD_MUTEX_INITIALIZER;

mytimer::mytimer(shared_ptr<requestData> _request_data, int timeout): deleted(false), request_data(_request_data)
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
    expired_time = jisuan(now.tv_sec,now.tv_usec,timeout);
   // cout<<expired_time<<endl;
}

mytimer::~mytimer()
{
    cout<<request_data<<endl;
    if (request_data)
    {
        Epoll::epoll_del(request_data->getFd(), EPOLLIN | EPOLLET | EPOLLONESHOT);
    }
}

void mytimer::update(int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    expired_time = jisuan(now.tv_sec,now.tv_usec,timeout);
}

bool mytimer::isvalid()
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
        this->setDeleted();
        return false;
    }
}

void mytimer::clearReq()
{
    request_data.reset();
    this->setDeleted();
}

void mytimer::setDeleted()
{
    deleted = true;
}

bool mytimer::isDeleted() const
{
    return deleted;
}

size_t mytimer::getExpTime() const
{
    return expired_time;
}

bool timerCmp::operator()(shared_ptr<mytimer> &a, shared_ptr<mytimer> &b) const
{
    return a->getExpTime() > b->getExpTime();
}
