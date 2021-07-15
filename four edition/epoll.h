#ifndef EVENTPOLL
#define EVENTPOLL
#include "requestdata.h"
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>
#include <vector>


class Epoll
{
private:
    static pthread_mutex_t lock;
    static epoll_event *events;
    static std::unordered_map<int, std::shared_ptr<RequestData>> fd2req;
    static int epoll_fd;
    static const std::string PATH;
public:
    static int EpollInit(int maxevents, int listen_num);
    static int EpollAdd(int fd, std::shared_ptr<RequestData> request, __uint32_t events);
    static int EpollMod(int fd, std::shared_ptr<RequestData> request, __uint32_t events);
    static int EpollDel(int fd, __uint32_t events);
    static void MyEpollWait(int listen_fd, int max_events, int timeout);
    static void AcceptConnection(int listen_fd, int epoll_fd, const std::string path);
    static std::vector<std::shared_ptr<RequestData>> GetEventsRequest(int listen_fd, int events_num, const std::string path);
};
#endif