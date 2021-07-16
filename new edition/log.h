#ifndef LOG
#define LOG

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <string>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "heaptimer.h"


class Log
{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *GetInstance()
    {
        static Log instance;
        return &instance;
    }

    static void *FlushLogThread(void *args)
    {
        Log::GetInstance()->AsyncWriteLog();
    }
    //可选择的参数有日志文件(file_name)、是否关闭日志(close_log)、日志缓冲区大小(log_buf_size)、最大行数(split_lines)以及最长日志条队列
    bool Init(const char *file_name, int close_log, int log_buf_size =2000, int split_lines =800000, int max_queue_size=1024);

    void WriteLog(int level, const char *format, ...);

    void Flush();

private:
    Log();
    virtual ~Log();
    void *AsyncWriteLog()
    {
        std::string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128]; //路径名
    char log_name[128]; //log文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count;  //日志行数记录
    int m_today;        //因为按天分类,记录当前时间是那一天
    FILE *m_fp;         //打开log的文件指针
    char *m_buf;
    BlockQueue<std::string> *m_log_queue; //阻塞队列
    bool m_is_async;                  //是否同步标志位
    locker m_mutex;
    int m_close_log; //关闭日志
};


#define LOG_DEBUG(format, ...) do{Log::GetInstance()->WriteLog(0, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}while(false);
#define LOG_INFO(format, ...) do{Log::GetInstance()->WriteLog(1, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}while(false);
#define LOG_WARN(format, ...) do{Log::GetInstance()->WriteLog(2, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}while(false);
#define LOG_ERROR(format, ...) do{Log::GetInstance()->WriteLog(3, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}while(false);

#endif