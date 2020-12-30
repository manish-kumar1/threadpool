#include "gtest/gtest.h"
#include "include/task_type.hpp"

namespace {

unsigned factorial(unsigned n){
  if (n < 2) return 1;
  else return n*factorial(n-1);
}

TEST(PriorityTaskTest, order) {
  thp::priority_task<unsigned, int> t1{factorial, 10}, t2{factorial, 0};
  thp::priority_task<unsigned, int> t3{factorial, 10};
  t1.set_priority(10);
  t2.set_priority(0);
  t3.set_priority(10);

  EXPECT_TRUE(t2 < t1);
  EXPECT_FALSE(t2 > t1);
  //EXPECT_FALSE(t2 == t1);
  EXPECT_FALSE(t3 < t1);
}

TEST(PriorityTaskTest, qorder) {

}

} // namespace
