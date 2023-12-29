#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <iostream>
#include <mutex>
#include <string_view>
#include <syncstream>
#include <thread>

template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& value) ;
    T pop() ;
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif