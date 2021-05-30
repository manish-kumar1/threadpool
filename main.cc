#include <iostream>
#include <string>
#include <chrono>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "spdlog/spdlog.h"

#include "include/threadpool.hpp"

using namespace std;
using namespace std::chrono;
namespace rng = std::ranges;

template<typename T>
void print_stats(T& vals) {
  vector<long int> data;
  auto n = vals.size();
  rng::transform(vals, std::back_inserter(data), [](auto&& f) { return f.get(); });
  //rng::copy(data, ostream_iterator<long int>(cout, "\n"));
  uint64_t total = std::accumulate(data.begin(), data.end(), uint64_t(0));
  auto [mn, mx] = rng::minmax_element(data);
  cout << total/n << " us" << endl;
  cout << *mn << ", " << *mx << endl;
  cout << distance(data.begin(), mn) << ", " << distance(data.begin(), mx) << endl;
}

void system_async(size_t n) {
  std::vector<std::future<long int>> vals;
  vals.reserve(n);

  for(int i = 0; i < n; ++i) {
    auto x = steady_clock::now();
    vals.emplace_back(std::async(std::launch::async, [x] { auto y = steady_clock::now(); return duration_cast<microseconds>(y-x).count(); }));
  }
  print_stats(vals);
}

void tp_enqueue(size_t n, size_t w) {
  thp::threadpool tp(w);
  std::vector<std::future<long int>> vals;
  vals.reserve(n);

  for(int i = 0; i < n; ++i) {
    auto x = steady_clock::now();
    auto [f] = tp.schedule(thp::make_task([x] { auto y = steady_clock::now(); return duration_cast<microseconds>(y-x).count(); }));
    vals.emplace_back(std::move(f));
  }
  print_stats(vals);
}

void tp_schedule(size_t n, size_t w) {
  thp::threadpool tp(w);
  std::vector<thp::simple_task<long int>> tasks;
  tasks.reserve(n);

  for(int i = 0; i < n; ++i) {
    auto x = steady_clock::now();
    tasks.emplace_back(thp::make_task([x] { auto y = steady_clock::now(); return duration_cast<microseconds>(y-x).count(); }));
  }
  auto [vals] = tp.schedule(std::move(tasks));
  print_stats(vals);
}

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  //LOG(INFO) << "Hello from threadpool!";

  spdlog::info("hello from threadpool");
  auto n = argc > 1 ? stoi(argv[1]) : 10000;
  auto w = argc > 2 ? stoi(argv[2]) : 16;

//  tp_schedule(n, w);
  tp_enqueue(n, w);
//  system_async(n);

  return 0;
}
