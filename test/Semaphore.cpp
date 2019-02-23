#include "./tests.hpp"

#include <thread>

#include "Semaphore.hpp"

using namespace std;
using namespace Catch::Matchers;

std::mutex m;
void
thread_wait(Semaphore* sem, int* value)
{
  sem->wait(1);
  m.lock();
  *value = *value + 1;
  m.unlock();
}

void
thread_sleep_wait(Semaphore* sem, int* value)
{
  m.lock();
  *value = *value - 1;
  m.unlock();
  this_thread::sleep_for(100ms);
  sem->wait(1);
  m.lock();
  *value = *value + 1;
  m.unlock();
}

void
thread_notify(Semaphore* sem, int* value)
{
  m.lock();
  *value = *value - 1;
  m.unlock();
  sem->notify(1);
  m.lock();
  *value = *value + 1;
  m.unlock();
}

void
thread_sleep_notify(Semaphore* sem, int* value)
{
  m.lock();
  *value = *value - 1;
  m.unlock();
  this_thread::sleep_for(100ms);
  sem->notify(1);
  m.lock();
  *value = *value + 1;
  m.unlock();
}

TEST_CASE("Semaphore", "[Semaphore]")
{

  SECTION("Semaphore(1) wait(1) waits for notify(1) (until is notified)")
  {
    Semaphore sem(1);

    int value = 0;
    thread t1(thread_wait, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 0);
    m.unlock();
    sem.notify(1);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 1);
    m.unlock();
    t1.join();
  }

  SECTION("Semaphore(1) notify(1) before wait(1) does the notification correctly")
  {
    Semaphore sem(1);

    int value = 1;
    thread t1(thread_sleep_wait, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 0);
    m.unlock();
    sem.notify(1);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 1);
    m.unlock();
    t1.join();
  }

  SECTION("Semaphore(2) wait(2) waits for 2 notify(1) (barrier type)")
  {
    Semaphore sem(2);

    int value = 2;
    thread t1(thread_sleep_notify, &sem, &value);
    thread t2(thread_sleep_notify, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 0);
    m.unlock();
    sem.wait(2);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 2);
    m.unlock();
    t1.join();
    t2.join();
  }

  SECTION("Semaphore(2) wait(2) waits for 2 notify(1) (barrier type, one after other)")
  {
    Semaphore sem(2);

    int value = 2;
    thread t1(thread_notify, &sem, &value);
    thread t2(thread_sleep_notify, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 1);
    m.unlock();
    sem.wait(2);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 2);
    m.unlock();
    t1.join();
    t2.join();
  }

  SECTION("Semaphore(1) with 2 wait(1) and 2 notify(1) each notify releases one wait")
  {
    Semaphore sem(1);

    int value = 2;
    thread t1(thread_sleep_wait, &sem, &value);
    thread t2(thread_sleep_wait, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 0);
    m.unlock();
    sem.notify(1);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 1);
    m.unlock();
    sem.notify(1);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 2);
    m.unlock();
    t1.join();
    t2.join();
  }

  SECTION("Semaphore(1) with 1 wait(1) and 1 notify(2) releases both waits")
  {
    Semaphore sem(1);

    int value = 2;
    thread t1(thread_sleep_wait, &sem, &value);
    thread t2(thread_sleep_wait, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 0);
    m.unlock();
    sem.notify(2);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 2);
    m.unlock();
    t1.join();
    t2.join();
  }

  SECTION("Semaphore(1) with 1 wait(1) and 1 notify(2) releases both waits (mixing)")
  {
    Semaphore sem(1);

    int value = 4;
    thread t1(thread_wait, &sem, &value);
    thread t2(thread_wait, &sem, &value);
    thread t3(thread_sleep_wait, &sem, &value);
    thread t4(thread_sleep_wait, &sem, &value);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 2);
    m.unlock();
    sem.notify(1);
    sem.notify(1);
    this_thread::sleep_for(50ms);
    m.lock();
    REQUIRE(value == 4);
    m.unlock();
    sem.notify(2);
    this_thread::sleep_for(100ms);
    m.lock();
    REQUIRE(value == 6);
    m.unlock();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
  }
}
