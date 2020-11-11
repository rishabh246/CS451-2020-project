#pragma once
#include <algorithm>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <vector>

/***** Thread safe vector *****/

template <typename T> class SharedVector {
private:
  std::mutex _m;
  std::vector<T> _vec;

public:
  SharedVector();
  ~SharedVector();
  void push_back(const T &item);
  bool
  remove(const T &item); /* Returns true if item existed, false otherwise */
  std::vector<T> get_items();
};

template <typename T> SharedVector<T>::SharedVector() {}

template <typename T> SharedVector<T>::~SharedVector() {}

template <typename T> void SharedVector<T>::push_back(const T &item) {
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

/***** Thread safe Map *****/

template <typename Key, typename Val> class SharedMap {
private:
  std::mutex _m;
  std::map<Key, Val> _map;

public:
  SharedMap() {}
  ~SharedMap() {}
  void insert(const Key &key, const Val &val) {
    std::lock_guard<std::mutex> lock(_m);
    _map[key] = val;
  }
  bool exists(const Key &key) {
    std::lock_guard<std::mutex> lock(_m);
    return _map.find(key) == _map.end();
  }
  bool remove(const Key &key)
  /* Returns true if item existed, false otherwise */ {
    std::lock_guard<std::mutex> lock(_m);
    auto pos = _map.find(key);
    if (pos == _map[key].end())
      return false;
    else
      _map.erase(pos);
  }
};

/***** Thread safe Map *****/

/***** Thread safe Map where all values are vectors
 *  This is actually quite redundant, and should be merged in SharedMap *****/

template <typename Key, typename Val> class SharedMapVec {
private:
  std::mutex _m;
  std::map<Key, std::vector<Val>> _map;

public:
  SharedMapVec() {}
  ~SharedMapVec() {}
  void insert_item(const Key &key, const Val &val) {
    std::lock_guard<std::mutex> lock(_m);
    if (_map.find(key) != _map.end())
      _map[key].push_back(val);
    else
      _map[key] = std::vector<Val>{val};
  }
  bool exists(const Key &key, const Val &val) {
    /* IMPORTANT: This action is not atomic when combined with an insert or
    remove. Hence, race conditions exist!! However, the use-case for this
    particular program avoids them */
    std::lock_guard<std::mutex> lock(_m);
    if (_map.find(key) == _map.end())
      return false;
    auto pos = std::find(_map[key].begin(), _map[key].end(), val);
    if (pos == _map[key].end())
      return false;
    return true;
  }

  bool remove(const Key &key, const Val &val)
  /* Returns true if item existed, false otherwise */ {
    std::lock_guard<std::mutex> lock(_m);
    if (_map.find(key) == _map.end())
      return false;
    auto pos = std::find(_map[key].begin(), _map[key].end(), val);
    if (pos == _map[key].end())
      return false;
    else
      _map[key].erase(pos);
    return true;
  }
  std::map<Key, std::vector<Val>> get_copy() {
    std::lock_guard<std::mutex> lock(_m);
    std::map<Key, std::vector<Val>> curr_items;
    curr_items.insert(_map.begin(), _map.end());
    return curr_items;
  }
};

/***** Thread safe Map *****/

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
