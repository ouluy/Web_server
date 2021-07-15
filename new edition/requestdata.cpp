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


pthread_mutex_t RequestData::lock = PTHREAD_MUTEX_INITIALIZER;

std::unordered_map<std::string, std::string> MimeType::my;
pthread_once_t MimeType::once= PTHREAD_ONCE_INIT;

void MimeType::Init(){
    my[".html"] = "text/html";
    my[".avi"] = "video/x-msvideo";
    my[".bmp"] = "image/bmp";
    my[".c"] = "text/plain";
    my[".doc"] = "application/msword";
    my[".gif"] = "image/gif";
    my[".gz"] = "application/x-gzip";
    my[".htm"] = "text/html";
    my[".ico"] = "application/x-ico";
    my[".jpg"] = "image/jpeg";
    my[".png"] = "image/png";
    my[".txt"] = "text/plain";
    my[".mp3"] = "audio/mp3";
    my["default"] = "text/html";
}

std::string MimeType::GetMime(const std::string &suffix){
    pthread_once(&once,MimeType::Init);
    if (my.find(suffix) == my.end())
        return my["default"];
    else
        return my[suffix];
}


std::priority_queue<shared_ptr<MyTimer>, std::deque<shared_ptr<MyTimer>>, TimerCmp> myTimerQueue;

RequestData::RequestData(): 
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), 
    keep_alive(false), againTimes(0)
{
    cout << "requestData constructed !" << endl;
}

RequestData::RequestData(int _epollfd, int _fd, std::string _path):
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), 
    keep_alive(false), againTimes(0),
    path(_path), fd(_fd), epollfd(_epollfd)
{}

RequestData::~RequestData(){
  //  cout << "~requestData()" << endl;
    close(fd);
}


void RequestData::AddTimer(std::shared_ptr<MyTimer> mtimer){
    // shared_ptr重载了bool, 但weak_ptr没有
    timer = mtimer;
}

int RequestData::GetFd(){
    return fd;
}

void RequestData::SetFd(int _fd){
    fd = _fd;
}

void RequestData::Reset()
{
    againTimes = 0;
    content.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PARSE_URI;
    h_state = h_start;
    headers.clear();
    keep_alive = false;
    if (timer.lock())
    {
        shared_ptr<MyTimer> my_timer(timer.lock());
        my_timer->ClearReq();
        timer.reset();
    }
}

void RequestData::SeperateTimer()
{
      if (timer.lock()){
        shared_ptr<MyTimer> my_timer(timer.lock());
        my_timer->ClearReq();
        timer.reset();
    }
}

void RequestData::HandleRequest()
{
    char buff[MAX_BUFF];
    bool isError = false;
    do//状态机
    {
       // cout<<"fd:"<<fd<<endl;
        int read_num = Readn(fd, buff, MAX_BUFF);

        //cout<<"read_num:"<<read_num<<endl;
        if (read_num < 0)
        {
            perror("1");
            isError = true;
            HandleError(fd,400,"Bad Request");
            break;
        }
        else if (read_num == 0)
        {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            /*perror("read_num==0");
            if (errno == EAGAIN)
            {
                if (againTimes >10) //AGAIN_MAX_TIMES)
                    isError = true;
                else
                    ++againTimes;//因为内测关闭againTimes,AGAIN_MAX_TIMES=200;
            }
            else if (errno != 0)*/
            //bug起因:但项目发现了一个bug，开启服务器直接webbench测试会core dumped(fd=5 forever),但如果开启服务器，联网进入目录之后,再webbench测压就不会出错,找了两天找不到bug在哪，怀疑哪个地方的指针写烂了
            //bug居然在这里
            // 一般对端已经关闭了，统一按照对端已经关闭处理
            isError = true;
            break;
        }
        string now_read(buff, buff + read_num);

        cout<<"nowread:\n"<<now_read<<"over\n"<<endl;

        content+=now_read;

        if (state == STATE_PARSE_URI)  //状态解析URI
        {
            int flag = this->ParseURI();
            if (flag == PARSE_URI_AGAIN)
            {
                break;
            }
            else if (flag == PARSE_URI_ERROR)
            {
                perror("2");
                isError = true;
                break;
            }
        }
        if (state == STATE_PARSE_HEADERS) //状态分析header
        {
            int flag = this->ParseHeaders();
            if (flag == PARSE_HEADER_AGAIN)
            {  
                break;
            }
            else if (flag == PARSE_HEADER_ERROR)
            {
                perror("3");
                isError = true;
                HandleError(fd, 400, "Bad Request");
                break;
            }
            if(method == METHOD_POST)//post 传两次
            {
                state = STATE_RECV_BODY;
            }
            else // get
            {
                state = STATE_ANALYSIS;
            }
        }
        if (state == STATE_RECV_BODY) //状态接收body
        {
            int content_length = -1;
            if (headers.find("Content-length") != headers.end())
            {
                content_length = stoi(headers["Content-length"]);
            }
            else
            {
                isError = true;
                HandleError(fd, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (content.size() < content_length)
                continue;
            state = STATE_ANALYSIS;
        }
        if (state == STATE_ANALYSIS)// 状态分析
        {
            int flag = this->AnalysisRequest();
            if (flag < 0)
            {
                isError = true;
                break;
            }
            else if (flag == ANALYSIS_SUCCESS)
            {

                state = STATE_FINISH;
                break;
            }
            else
            {
                isError = true;
                break;
            }
        }
    }while(false);
    
    if (isError)
    {
        return;
    }
    // 加入epoll继续
    

    if (state == STATE_FINISH){ //状态完成
        if (keep_alive)
        {
            printf("ok\n");
            this->Reset();
        }
        else
        {
            return;
        }
    }
    // 一定要先加时间信息，否则可能会出现刚加进去，下个in触发来了，然后分离失败后，又加入队列，最后超时被删，然后正在线程中进行的任务出错，double free错误。
    // 新增时间信息
   
    shared_ptr<MyTimer> mtimer(new MyTimer(shared_from_this(),1000));

    pthread_mutex_lock(&lock);

    myTimerQueue.push(mtimer);//最小堆定时器

    pthread_mutex_unlock(&lock);

    this->AddTimer(mtimer);


    __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;

    int ret = Epoll::EpollMod(fd, shared_from_this(), _epo_event);
    
  //  printf("ret: %d\n",ret);
    
    if (ret < 0){
        // 返回错误处理
        return;
    }
    
}

int RequestData::ParseURI()
{
    string &str = content;

    // 读到完整的请求行再开始解析请求
    int pos = str.find('\r', now_read_pos);
    if (pos < 0)
    {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else 
        str.clear();
    // Method
    pos = request_line.find("GET");
    if (pos < 0)
    {
        pos = request_line.find("POST");
        if (pos < 0)
        {
            return PARSE_URI_ERROR;
        }
        else
        {
            method = METHOD_POST;
        }
    }
    else
    {
        method = METHOD_GET;
    }
   // printf("method = %d\n", method);
    // filename
    pos = request_line.find("/", pos);
    if (pos < 0)
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        int _pos = request_line.find(' ', pos);
        if (_pos < 0)
            return PARSE_URI_ERROR;
        else
        {
            if (_pos - pos > 1)
            {
                file_name = request_line.substr(pos + 1, _pos - pos - 1);
                int __pos = file_name.find('?');
                if (__pos >= 0)
                {
                    file_name = file_name.substr(0, __pos);
                }
             //   cout<<"file_name:"<<file_name<<endl;
            }
                
            else{
                file_name = "ouluy.html";
            }
        }
        pos = _pos;
    }
   // cout << "file_name: " << file_name << endl;
    // HTTP 版本号
    pos = request_line.find("/", pos);
    if (pos < 0)
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        if (request_line.size() - pos <= 3)
        {
            return PARSE_URI_ERROR;
        }
        else
        {
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                HTTPversion = HTTP_10;
            else if (ver == "1.1")
                HTTPversion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    state = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

int RequestData::ParseHeaders()
{
    string &str = content;

    //cout<<str<<endl;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;

    int now_read_line_begin = 0;

    bool notFinish = true;

    for (int i = 0; i < str.size() && notFinish; ++i)
    {
        switch(h_state)
        {
            case h_start:
            {
                if (str[i] == '\n' || str[i] == '\r')
                    break;
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case h_key:
            {
                if (str[i] == ':')
                {
                    key_end = i;
                    if (key_end - key_start <= 0)
                        return PARSE_HEADER_ERROR;
                    h_state = h_colon;
                }
                else if (str[i] == '\n' || str[i] == '\r')
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_colon:
            {
                if (str[i] == ' ')
                {
                    h_state = h_spaces_after_colon;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_spaces_after_colon:
            {
                h_state = h_value;
                value_start = i;
                break;  
            }
            case h_value:
            {
                if (str[i] == '\r')
                {
                    h_state = h_CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                }
                else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_CR:
            {
                if (str[i] == '\n')
                {
                    h_state = h_LF;
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_LF:
            {
                if (str[i] == '\r')
                {
                    h_state = h_end_CR;
                }
                else
                {
                    key_start = i;
                    h_state = h_key;
                }
                break;
            }
            case h_end_CR:
            {
                if (str[i] == '\n')
                {
                    h_state = h_end_LF;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_end_LF:
            {
                notFinish = false;
                now_read_line_begin = i;
                break;
            }
        }
    }
    if (h_state == h_end_LF)
    {
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}


int RequestData::AnalysisRequest()
{

    if (method == METHOD_POST)
    {

        //get content
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        //cout << "content=" << content << endl;
        // test char*
        char *send_content = "I have receiced this.";

        sprintf(header, "%sContent-length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        size_t send_len = (size_t)Writen(fd, header, strlen(header));
        if(send_len != strlen(header))
        {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        
        send_len = (size_t)Writen(fd, send_content, strlen(send_content));
        if(send_len != strlen(send_content))
        {
            perror("Send content failed");
            return ANALYSIS_ERROR;
        }
        //cout << "content size ==" << content.size() << endl;
        //vector<char> data(content.begin(), content.end());
        //Mat test = imdecode(data, CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_ANYCOLOR);
       // imwrite("receive.bmp", test);
        return ANALYSIS_SUCCESS;
    }
    else if (method == METHOD_GET)
    {
        //cout<<"file_name:"<<file_name<<endl;
        //if(file_name=="ouluy.html") printf("is mu lu\n");
            
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive"){
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        int dot_pos = file_name.find('.');
        const char* filetype;
        if (dot_pos < 0) 
            filetype = MimeType::GetMime("default").c_str();
        else
            filetype = MimeType::GetMime(file_name.substr(dot_pos)).c_str();

        struct stat sbuf;
        if (stat(file_name.c_str(), &sbuf) < 0){
            HandleError(fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }

        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        // 通过Content-length返回文件大小
        sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);

        sprintf(header, "%s\r\n", header);
        size_t send_len = (size_t)Writen(fd, header, strlen(header));
        if(send_len != strlen(header)){
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        int src_fd = open(file_name.c_str(), O_RDONLY, 0);

        char *src_addr = static_cast<char*>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        
        close(src_fd);
    
        // 发送文件并校验完整性
        send_len = Writen(fd, src_addr, sbuf.st_size);
        if(send_len != sbuf.st_size){
            perror("Send file failed");
            return ANALYSIS_ERROR; 
        }
        
        int endd=munmap(src_addr, sbuf.st_size);// 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0 成功执行时，mmap()返回被映射区的指针，munmap()返回0
        //该调用在进程地址空间中解除一个映射关系
        if(endd!=0){
            perror("mmap failure");
        }
        return ANALYSIS_SUCCESS;
    }
    else
        return ANALYSIS_ERROR;
}

void RequestData::HandleError(int fd, int err_num, string short_msg)
{   //404
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    string body_buff, header_buff;
    body_buff += "<html><title>Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> ouluy's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";
    sprintf(send_buff, "%s", header_buff.c_str());
    Writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    Writen(fd, send_buff, strlen(send_buff));
}
