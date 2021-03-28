#include <iostream>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "spdlog/spdlog.h"

#include "include/threadpool.hpp"

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  //LOG(INFO) << "Hello from threadpool!";

  spdlog::info("hello from threadpool");

  thp::threadpool tp;

  tp.enqueue([] { std::cerr << "Hello world" << std::endl; });
  tp.drain();
  tp.shutdown();

  return 0;
}
