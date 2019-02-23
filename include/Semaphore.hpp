/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SEMAPHORE_HPP
#define ENGINECL_SEMAPHORE_HPP 1

#include <condition_variable>
#include <iostream>
#include <mutex>

/*!
 * Semaphore use cases:
 *
 * **1-1 as a releaser**
 *
 * ```cpp
 * void thread_wait(semaphore* sem){
 *   sem->wait(1); // waits until main does sem.notify(1)
 * }
 *
 * void main(){
 *   semaphore sem(1);
 *
 *   thread t1(thread_wait, &sem);
 *
 *   sem.notify(1); // never stops
 *
 *   t1.join();
 * }
 * ```
 *
 * It works if main reaches `notify` before t1 the `wait` and if t1 reaches
 * `wait` before main the `notify`.
 *
 * **4-1-1-2 as a releaser**
 *
 * ```cpp
 * void thread_notify(semaphore* sem){
 *   sem->wait(1);
 * }
 *
 * void main(){
 *   semaphore sem(1); // important, only 1
 *
 *   thread t1(thread_notify, &sem);
 *   thread t2(thread_notify, &sem);
 *   thread t3(thread_notify, &sem);
 *   thread t4(thread_notify, &sem);
 *
 *   sem.notify(1); // releases one thread
 *   sem.notify(1); // releases other thread
 *   sem.notify(2); // releases 2 threads
 *
 *   t1.join();
 *   t2.join();
 *   t3.join();
 *   t4.join();
 * }
 * ```
 *
 * **2-1-1 as a barrier**
 *
 * ```cpp
 * void thread_notify(semaphore* sem){
 *   sem->notify(1);
 * }
 *
 * void main(){
 *   semaphore sem(2); // important, here 2
 *
 *   thread t1(thread_notify, &sem);
 *   thread t2(thread_notify, &sem);
 *
 *   sem.wait(2); // waits until both threads notify
 *
 *   t1.join();
 *   t2.join();
 * }
 * ```
 *
 */

template<typename Mutex, typename CondVar>
class basic_semaphore
{
public:
  using native_handle_type = typename CondVar::native_handle_type;

  explicit basic_semaphore(int count = 0);
  basic_semaphore(const basic_semaphore&) = delete;
  basic_semaphore(basic_semaphore&&) = delete;
  basic_semaphore& operator=(const basic_semaphore&) = delete;
  basic_semaphore& operator=(basic_semaphore&&) = delete;

  void notify(int count = 1);
  void wait(int count = 1);
  bool available();
  bool try_wait();
  template<class Rep, class Period>
  bool wait_for(const std::chrono::duration<Rep, Period>& d);
  template<class Clock, class Duration>
  bool wait_until(const std::chrono::time_point<Clock, Duration>& t);

  native_handle_type native_handle();

private:
  Mutex mMutex;
  CondVar mCv;
  int mCount;
};

using Semaphore = basic_semaphore<std::mutex, std::condition_variable>;

template<typename Mutex, typename CondVar>
basic_semaphore<Mutex, CondVar>::basic_semaphore(int count)
  : mCount{ -count }
{}

/*!
  \brief consume `count` resources and notify one/all threads (all if count >
  1). \param count number of resources to consume
*/
template<typename Mutex, typename CondVar>
void
basic_semaphore<Mutex, CondVar>::notify(int count)
{
  std::lock_guard<Mutex> lock{ mMutex };
  mCount += count;
  if (count > 1) {
    mCv.notify_all();
  } else {
    mCv.notify_one();
  }
}

/*!
  \brief wait if no free resources and release `count` resources after being
  awaken. \param count number of resources to free
 */
template<typename Mutex, typename CondVar>
void
basic_semaphore<Mutex, CondVar>::wait(int count)
{
  std::unique_lock<Mutex> lock{ mMutex };
  // releases the lock when waiting (waits always when mCount is <= 0)
  mCv.wait(lock, [&] { return mCount >= 0; });
  mCount -= count;
}

template<typename Mutex, typename CondVar>
bool
basic_semaphore<Mutex, CondVar>::try_wait()
{
  std::lock_guard<Mutex> lock{ mMutex };

  if (mCount > 0) {
    --mCount;
    return true;
  }

  return false;
}

template<typename Mutex, typename CondVar>
bool
basic_semaphore<Mutex, CondVar>::available()
{
  std::lock_guard<Mutex> lock{ mMutex };

  return mCount > 0;
}

template<typename Mutex, typename CondVar>
template<class Rep, class Period>
bool
basic_semaphore<Mutex, CondVar>::wait_for(const std::chrono::duration<Rep, Period>& d)
{
  std::unique_lock<Mutex> lock{ mMutex };
  auto finished = mCv.wait_for(lock, d, [&] { return mCount > 0; });

  if (finished)
    --mCount;

  return finished;
}

template<typename Mutex, typename CondVar>
template<class Clock, class Duration>
bool
basic_semaphore<Mutex, CondVar>::wait_until(const std::chrono::time_point<Clock, Duration>& t)
{
  std::unique_lock<Mutex> lock{ mMutex };
  auto finished = mCv.wait_until(lock, t, [&] { return mCount > 0; });

  if (finished)
    --mCount;

  return finished;
}

template<typename Mutex, typename CondVar>
typename basic_semaphore<Mutex, CondVar>::native_handle_type
basic_semaphore<Mutex, CondVar>::native_handle()
{
  return mCv.native_handle();
}

#endif // ENGINECL_SEMAPHORE_HPP
