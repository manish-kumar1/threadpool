#include <iostream>
#include <string>
#include <random>
#include <tuple>
#include <csignal>
#include <chrono>
#include <memory>
#include <experimental/filesystem>
#include <glog/logging.h>

#include "include/threadpool.hpp"

using namespace std::chrono;
using namespace thp;


int exception_thread() {
  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_int_distribution<int> dis(0, std::numeric_limits<int>::max());
  std::this_thread::sleep_for(milliseconds(2000));
  auto v = dis(e);
  if (v % 2 == 0) {
    std::cerr << "exception_thread(): throwing exception" << std::endl;
    throw std::logic_error("exception_thread");
  }
  return v;
}

float segfault_function() {
  int x = 0;
  return 1/x;
}

decltype(auto) check_prime(const unsigned times, threadpool& tp) {
  std::vector<std::pair<unsigned int, std::future<bool>>> ret;
  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_int_distribution<unsigned int> dis(900000000, std::numeric_limits<unsigned int>::max());
  std::uniform_int_distribution<unsigned int> ts(0, 10);

  ret.reserve(times);

  for (unsigned int i = 0; i < times; ++i) {
    unsigned int num = dis(e);
    // create random types of jobs at runtime
    	auto [fut1] = tp.schedule(is_prime, num);
    	ret.emplace_back(num, std::move(fut1));
    	auto [fut2] = tp.schedule(util::make_task<float>(is_prime, num));
    	ret.emplace_back(num, std::move(fut2));
    	auto t = util::make_task<system_clock::time_point>(is_prime, num);
      auto [fut3] = tp.schedule(std::move(t));
    	ret.emplace_back(num, std::move(fut3));
    	auto [fut4] = tp.schedule(util::make_task<int>(is_prime, num));
    	ret.emplace_back(num, std::move(fut4));
  }
  std::cerr << "check_prime created" << std::endl;
  return ret;
}

void print_primes(std::future<std::vector<std::pair<unsigned int, std::future<bool>>>> f) {
  // std::vector<std::shared_future<bool>> futs;
  // std::transform(primes.begin(), primes.end(), std::back_inserter(futs), [](auto& p) { return p.second;});
  // auto ans = std::when_all(futs.begin(), futs.end());
  // ans.wait();
  std::cerr << "primes = [";
  for(auto&& p : f.get())
    if (p.second.get())
      std::cerr << p.first << ", ";
  std::cerr << "]" << std::endl;
}

unsigned factorial(unsigned n) {
	if (n < 2) return 1;
	else return n*factorial(n-1);
}

int main(int argc, const char* const argv[])
{
  //google::InitGoogleLogging(argv[0]);

  threadpool tp;

  auto p0 = util::make_task([] { std::cerr << "Hello world" << std::endl; });
  auto p1 = util::make_task<int>(factorial, 100); p1->set_priority(10);
  auto p2 = util::make_task<float>(factorial, 20); p2->set_priority(25.5f);
  auto p3 = util::make_task(check_prime, 100, std::ref(tp));

  auto [f0, f1, f2] = tp.schedule(p0,
                                  p1,
                                  p2,
                                  );

  auto [f3] = tp.schedule(p3);
  print_primes(std::move(f3));

  auto [f4] = tp.enqueue(check_prime, 64, std::ref(tp));

  auto p7 = util::make_task<int>(exception_thread);
  tp.schedule(p7);

  tp.drain();

  auto p5 = util::make_task<system_clock::time_point>(check_prime, 1024, std::ref(tp)); p5->set_priority(system_clock::now()+5s);
  auto [f5] = tp.schedule(p5);
  print_primes(std::move(f5));

  std::vector<int> data = {1, 2, 3, 4, 5, 6, 10};
  tp.for_each(data.begin(), data.end(), [](int i) {
    std::cout << "factorial(" << i << ") = " << factorial(i) << std::endl;
  });


  tp.shutdown();
  std::cerr << "main exit.." << std::endl;

  return 0;
}
