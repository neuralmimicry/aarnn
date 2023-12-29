#include "../include/ThreadSafeQueue.h"

void ThreadSafeQueue::push(const T &value){
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(value);
    cond_.notify_one();
}

T ThreadSafeQueue::pop(){
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()){
        cond_.wait(lock);
    }
    T val = queue_.front();
    queue_.pop();
    return val;
}