#ifndef REQUESTDATA
#define REQUESTDATA

#include<string>
#include"heaptimer.h"
#include <memory>
#include<unordered_map>
#include <dirent.h>
#include <sys/stat.h>

const int STATE_PARSE_URI = 1;     //状态解析URI
const int STATE_PARSE_HEADERS = 2; //状态分析标头
const int STATE_RECV_BODY = 3;     //状态接收体
const int STATE_ANALYSIS = 4;      //状态分析
const int STATE_FINISH = 5;        //状态完成

const int MAX_BUFF = 4096;         //最大BUFF

// 有请求出现但是读不到数据,可能是Request Aborted,
// 或者来自网络的数据没有达到等原因,
// 对这样的请求尝试超过一定的次数就抛弃
const int AGAIN_MAX_TIMES = 100;   //再次最大次数

const int PARSE_URI_AGAIN = -1;    //再次解析URI
const int PARSE_URI_ERROR = -2;    //解析URI错误
const int PARSE_URI_SUCCESS = 0;   //解析URI成功

const int PARSE_HEADER_AGAIN = -1; //再次解析头
const int PARSE_HEADER_ERROR = -2; //解析头错误
const int PARSE_HEADER_SUCCESS = 0;//解析头成功

const int ANALYSIS_ERROR = -2;     //分析误差
const int ANALYSIS_SUCCESS = 0;    //分析成功

const int METHOD_POST = 1;         
const int METHOD_GET = 2;          
const int HTTP_10 = 1;
const int HTTP_11 = 2;   

const int EPOLL_WAIT_TIME = 500;  

class MimeType{
private:
    static void Init();
    static std::unordered_map<std::string,std::string>my;
    MimeType();
    MimeType(const MimeType &m);
    static pthread_once_t once;
public:
    static std::string GetMime(const std::string &suffix);
    
};

enum HeadersState{
    h_start = 0,
    h_key,
    h_colon,
    h_spaces_after_colon,
    h_value,
    h_CR,
    h_LF,
    h_end_CR,
    h_end_LF
};


struct MyTimer;


struct RequestData : public std::enable_shared_from_this<RequestData>
{
private:
    static pthread_mutex_t lock;
    int againTimes;
    std::string path;
    int fd;
    int epollfd;
    std::string content;
    int method;
    int HTTPversion;
    std::string file_name;
    int now_read_pos;
    int state;
    int h_state;
    bool isfinish;
    bool keep_alive;
   // locker m_mutex;
    std::unordered_map<std::string,std::string>headers;
    std::weak_ptr<MyTimer> timer;    
private:
    int ParseURI();
    int ParseHeaders();
    int AnalysisRequest();
public:
    RequestData();
    RequestData(int _epollfd,int _fd,std::string _path);
    ~RequestData();
    void AddTimer(std::shared_ptr<MyTimer> mtimer);
    void Reset();
    void SeperateTimer();
    int GetFd();
    void SetFd(int _fd);
    void HandleRequest();
    void HandleError(int fd, int err_num, std::string short_msg);
};

#endif