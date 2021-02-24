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
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "Hello from threadpool!";
  
  thp::threadpool tp;

  tp.enqueue([] { std::cerr << "Hello world" << std::endl; });
  tp.drain();
  tp.shutdown();

  return 0;
}
