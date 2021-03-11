#include <iostream>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "include/threadpool.hpp"

int main(int argc, const char* const argv[])
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
