#ifndef THREADSAFEQUEUE_H_INCLUDED
#define THREADSAFEQUEUE_H_INCLUDED

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string_view>
#include <syncstream>
#include <thread>

template<typename T>
class ThreadSafeQueue
{
    public:
    void push(const T &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        cond_.notify_one();
    }
    
    T pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(queue_.empty())
        {
            cond_.wait(lock);
        }
        T val = queue_.front();
        queue_.pop();
        return val;
    }

    private:
    std::queue<T>           queue_;
    std::mutex              mutex_;
    std::condition_variable cond_;
};

#endif  // THREADSAFEQUEUE_H_INCLUDED
