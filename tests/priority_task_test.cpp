#include <chrono>

#include "gtest/gtest.h"
#include "include/task_type.hpp"
#include "include/time_task.hpp"
#include "include/task_queue.hpp"
#include "include/task_factory.hpp"

namespace {

unsigned factorial(unsigned n){
  if (n < 2) return 1;
  else return n*factorial(n-1);
}

TEST(PriorityTaskTest, order) {
  using namespace std::chrono_literals;
  using namespace std::chrono;

  thp::priority_task<unsigned, int> t1{factorial, 10}, t2{factorial, 0};
  thp::priority_task<unsigned, int> t3{factorial, 10};
  thp::time_task<unsigned, steady_clock> t4(factorial, 10); 
  thp::time_task<unsigned, system_clock> t5(factorial, 20); 
  thp::time_task<unsigned, system_clock> t6(factorial, 20); 

  t4.set_priority(steady_clock::now()+4s);

  t5.set_priority(system_clock::now()+5s);
  t6.set_priority(system_clock::now()+10s);

  t1.set_priority(10);
  t2.set_priority(0);
  t3.set_priority(10);

  EXPECT_TRUE(t2 < t1);
  EXPECT_FALSE(t2 > t1);
  //EXPECT_FALSE(t2 == t1);
  EXPECT_FALSE(t3 < t1);

  EXPECT_FALSE(t4 < t4);
  //EXPECT_TRUE(t4 == t4); equality fails because of strong ordering

  EXPECT_TRUE(t5 < t6);
  EXPECT_FALSE(t5 >= t6);
}

TEST(PriorityTaskTest, qorder) {
  auto t1 = thp::make_task<int>(factorial, 1);
  auto t2 = thp::make_task<int>(factorial, 2);
  auto t3 = thp::make_task<int>(factorial, 3);
  auto f1 = t1->future(); 
  auto f2 = t2->future(); 
  auto f3 = t3->future(); 
  t1->set_priority(1);
  t2->set_priority(2);
  t3->set_priority(3);

  thp::priority_taskq<int> q;
  q.put(std::move(t3));
  q.put(std::move(t1));
  q.put(std::move(t2));
  std::shared_ptr<thp::executable> t;
  q.pop(t);
  t->execute();
  EXPECT_TRUE(f3.valid());
  EXPECT_EQ(6, f3.get());
}

} // namespace
