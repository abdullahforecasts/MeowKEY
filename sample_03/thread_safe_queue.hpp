#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T> 
class ThreadSafeQueue { 
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable condition_;  
    size_t max_size_;

public:
    ThreadSafeQueue(size_t max_size = 1000) : max_size_(max_size) {}

    void push(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.size() >= max_size_) {
            queue_.pop(); // Remove oldest if queue is full
        }
        queue_.push(std::move(value));
        condition_.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (condition_.wait_for(lock, std::chrono::milliseconds(100), 
                               [this] { return !queue_.empty(); })) {
            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }
        return std::nullopt;
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

#endif