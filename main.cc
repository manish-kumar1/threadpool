#include <iostream>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "include/threadpool.hpp"

auto print_future = [](auto &&fut) {
  try {
    auto v = fut.get();
    std::cerr << "fut::value= " << v << std::endl;
  } catch (std::exception& e) {
    std::cerr << "fut::exception= " << e.what() << std::endl;
  }
};

unsigned factorial(unsigned n) {
  if (n < 2) return 1;
  else return n*factorial(n-1);
}

// threadpool -thread_config='conf.yaml' --enable_print
int main(int argc, char* argv[])
{
//  gflags::ParseCommandLineFlags(&argc, &argv, false);

  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "Hello from threadpool!";
  
  thp::threadpool tp;
  // simple task<ReturnType>
  auto p0 = thp::make_task([] { std::cerr << "Hello world" << std::endl; });
  // task with priority setup
  auto p1 = thp::make_task<float>(factorial, 3); p1->set_priority(-30.5f);
  auto p2 = thp::make_task<float>(factorial, 4); p2->set_priority(-20.5f);

  std::vector<decltype(p1)> tasks;
  auto p3 = thp::make_task<float>(factorial, 5); p3->set_priority(-30.5f);
  tasks.emplace_back(std::move(p3));

  auto [f0, f1, f2] = tp.schedule(p0, p1, p2);
  auto futs = tp.schedule(std::move(tasks));

  tp.drain();
  tp.shutdown();

  return 0;
}
