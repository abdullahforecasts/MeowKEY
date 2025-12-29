#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
using namespace std;

template <typename T>
class ThreadSafeQueue
{
private:
  mutable mutex mutex_;
  queue<T> queue_;
  condition_variable condition_;
  size_t max_size_;

public:
  ThreadSafeQueue(size_t max_size = 1000) : max_size_(max_size) {}

  void push(T value)
  {
    unique_lock<mutex> lock(mutex_);
    if (queue_.size() >= max_size_)
    {
      queue_.pop(); // Remove oldest if queue is full
    }
    queue_.push(move(value));
    condition_.notify_one();
  }

  optional<T> pop()
  {
    unique_lock<mutex> lock(mutex_);
    if (condition_.wait_for(lock, chrono::milliseconds(100),
                            [this]
                            { return !queue_.empty(); }))
    {
      T value = move(queue_.front());
      queue_.pop();
      return value;
    }
    return nullopt;
  }

  bool empty() const
  {
    unique_lock<mutex> lock(mutex_);
    return queue_.empty();
  }

  size_t size() const
  {
    unique_lock<mutex> lock(mutex_);
    return queue_.size();
  }
};

#endif