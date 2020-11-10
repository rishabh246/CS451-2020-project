#pragma once
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>

/***** Thread safe vector *****/

template <typename T> class SharedVector {
private:
  std::mutex _m;
  std::vector<T> _vec;

public:
  SharedVector();
  ~SharedVector();
  void add(const T &item);
  bool
  remove(const T &item); /* Returns true if item existed, false otherwise */
  std::vector<T> get_items();
};

template <typename T> SharedVector<T>::SharedVector() {}

template <typename T> SharedVector<T>::~SharedVector() {}

template <typename T> void SharedVector<T>::add(const T &item) {
  std::lock_guard<std::mutex> lock(_m);
  _vec.push_back(item);
}

template <typename T> bool SharedVector<T>::remove(const T &item) {
  std::lock_guard<std::mutex> lock(_m);
  auto pos = std::find(_vec.begin(), _vec.end(), item);
  if (pos == _vec.end())
    return false;
  else
    _vec.erase(pos);
  return true;
}

template <typename T> std::vector<T> SharedVector<T>::get_items() {
  std::lock_guard<std::mutex> lock(_m);
  std::vector<T> curr_items;
  curr_items.insert(curr_items.end(), _vec.begin(), _vec.end());
  return curr_items;
}

/***** Thread safe vector *****/

/***** Thread safe queue *****/

/* From
 * https://stackoverflow.com/questions/36762248/why-is-stdqueue-not-thread-safe
 */

template <typename T> class SharedQueue {
public:
  SharedQueue();
  ~SharedQueue();

  T &front();
  void pop_front();

  void push_back(const T &item);
  void push_back(T &&item);

  int size();
  bool empty();

private:
  std::deque<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

template <typename T> SharedQueue<T>::SharedQueue() {}

template <typename T> SharedQueue<T>::~SharedQueue() {}

template <typename T> T &SharedQueue<T>::front() {
  std::unique_lock<std::mutex> mlock(mutex_);
  while (queue_.empty()) {
    cond_.wait(mlock);
  }
  return queue_.front();
}

template <typename T> void SharedQueue<T>::pop_front() {
  std::unique_lock<std::mutex> mlock(mutex_);
  while (queue_.empty()) {
    cond_.wait(mlock);
  }
  queue_.pop_front();
}

template <typename T> void SharedQueue<T>::push_back(const T &item) {
  std::unique_lock<std::mutex> mlock(mutex_);
  queue_.push_back(item);
  mlock.unlock();     // unlock before notificiation to minimize mutex con
  cond_.notify_one(); // notify one waiting thread
}

template <typename T> void SharedQueue<T>::push_back(T &&item) {
  std::unique_lock<std::mutex> mlock(mutex_);
  queue_.push_back(std::move(item));
  mlock.unlock();     // unlock before notificiation to minimize mutex con
  cond_.notify_one(); // notify one waiting thread
}

template <typename T> int SharedQueue<T>::size() {
  std::unique_lock<std::mutex> mlock(mutex_);
  int size = queue_.size();
  mlock.unlock();
  return size;
}

template <typename T> bool SharedQueue<T>::empty() { return size() == 0; }

/***** Thread safe queue ends *****/