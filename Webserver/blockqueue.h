/*************************************************************
*循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;  
*线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
**************************************************************/

#ifndef BLOCKQUEUE
#define BLOCKQUEUE

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockQueue {
public:
    explicit BlockQueue(size_t MaxCapacity = 1000);

    ~BlockQueue();

    void clear();

    bool empty();

    bool full();

    void Close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T &item);

    void push_front(const T &item);

    bool pop(T &item);

    bool pop(T &item, int timeout);

    void flush();

private:
    std::deque<T> deq_;

    size_t capacity_;

    std::mutex lock;

    bool isClose_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;
};


template<class T>
BlockQueue<T>::BlockQueue(size_t MaxCapacity) :capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);
    isClose_ = false;
}

template<class T>
BlockQueue<T>::~BlockQueue() {
    Close();
};

template<class T>
void BlockQueue<T>::Close() {
    {   
        std::lock_guard<std::mutex> locker(lock);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
};

template<class T>
void BlockQueue<T>::flush() {
    condConsumer_.notify_one();
};

template<class T>
void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> locker(lock);
    deq_.clear();
}

template<class T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> locker(lock);
    return deq_.front();
}

template<class T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> locker(lock);
    return deq_.back();
}

template<class T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> locker(lock);
    return deq_.size();
}

template<class T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> locker(lock);
    return capacity_;
}

template<class T>
void BlockQueue<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(lock);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
}

template<class T>
void BlockQueue<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(lock);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<class T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> locker(lock);
    return deq_.empty();
}

template<class T>
bool BlockQueue<T>::full(){
    std::lock_guard<std::mutex> locker(lock);
    return deq_.size() >= capacity_;
}

template<class T>
bool BlockQueue<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(lock);
    while(deq_.empty()){
        condConsumer_.wait(locker);
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<class T>
bool BlockQueue<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(lock);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

#endif