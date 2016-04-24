// Copyright 2013 15418 Course Staff.

#ifndef __WORKER_WORK_QUEUE_H__
#define __WORKER_WORK_QUEUE_H__


#include <vector>
std::mutex m_mutex;


template <class T>
class WorkQueue {
private:
  std::vector<T> storage;
  std::mutex queue_lock;
  std::condition_variable queue_cond;

public:

  WorkQueue() {

  }

  T get_work() {
    std::unique_lock<std::mutex> lck(queue_lock);
    while (storage.size() == 0) {
      queue_cond.wait(lck);
    }

    T item = storage.front();
    storage.erase(storage.begin());
    return item;
  }

  void put_work(const T& item) {
    std::unique_lock<std::mutex> lck(queue_lock);
    storage.push_back(item);
    queue_cond.notify_one();
  }

  bool check_and_get_work(T& item_container) {
    std::unique_lock<std::mutex> lck(queue_lock);
    if (storage.size() == 0) {
      return false;
    } else {
      item_container = storage.front();
      storage.erase(storage.begin());
      return true;
    }
  }
};

#endif  // WORKER_WORK_QUEUE_H_

class Barrier {
private:
  std::mutex barrier_lock;
  std::condition_variable cv;
  std::size_t max_count;
  std::size_t count;
  bool on;
public:
  explicit Barrier(std::size_t count) : max_count{count}, count{0}, on{false} { }
  void activate() {
    std::unique_lock<std::mutex> lock(barrier_lock);
    count = max_count;
    on = true;
  }
  void wait() {
    std::unique_lock<std::mutex> lock(barrier_lock);
    if (on) {
      if (--count == 0) {
        on = false;
        cv.notify_all();
      } else {
        cv.wait(lock);
      }
    }
  }
};