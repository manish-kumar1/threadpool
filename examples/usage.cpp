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

// very inefficient prime checker to eat up cpu
bool is_prime(unsigned n) {
  bool ret = true;
  if (n < 2)
    ret = false;
  else if (n == 2)
    ret = true;
  else if (n%2 == 0)
    ret = false;
  else {
    for (unsigned i = 3; i < static_cast<unsigned>(1 + sqrt(n)); i += 2) {
      if (n % i == 0) {
        ret = false;
        break;
      }
    }
  }
  return ret;
}

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
    	auto [fut1] = tp.enqueue(is_prime, num);
    	ret.emplace_back(num, std::move(fut1));
    	auto [fut2, fut3, fut4] = tp.schedule(make_task<float>(is_prime, num),
    	                                      make_task<system_clock::time_point>(is_prime, num),
    	                                      make_task<int>(is_prime, num));
      ret.emplace_back(num, std::move(fut2));
      ret.emplace_back(num, std::move(fut3));
      ret.emplace_back(num, std::move(fut4));
  }
  std::cerr << "check_prime created" << std::endl;
  return ret;
}

void print_primes(std::future<std::vector<std::pair<unsigned int, std::future<bool>>>>& f) {
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

  auto hello = [] { std::cerr << "hello world" << std::endl; };

  auto p0 = make_task(hello);
//  auto p1 = make_task<int>(factorial, 10); p1->set_priority(-42);
  priority_task<unsigned, int> p1(factorial, 10); p1.set_priority(-42);
//  auto x = priority_task<int>(factorial, 10).priority(-42).run_on(cpu1).after(p0).before(p3).for(3s);
//  auto x = priority_task<int>(factorial, 10).priority(-42).with_config(conf)
  auto p2 = make_task<float>(factorial, 20); p2->set_priority(25.5f);
  auto p3 = make_task(check_prime, 100, std::ref(tp));

  //std::list<simple_task<unsigned>> p4; // debug why list, deque crashes
  std::vector<simple_task<unsigned>> p4;
  p4.emplace_back(simple_task<unsigned>(factorial, 10));
  p4.emplace_back(simple_task<unsigned>(factorial, 20));

  auto [f1] = tp.schedule(p1);
  //print_primes(f3);

  auto f5 = tp.enqueue(check_prime, 64, std::ref(tp));
  auto p6 = tp.schedule(make_task<int>(exception_thread));

  tp.drain();

  auto p7 = make_task<system_clock::time_point>(check_prime, 1024, std::ref(tp)); p7->set_priority(system_clock::now()+5s);
  auto [f7] = tp.schedule(p7);
  print_primes(f7);

  std::vector<int> data = {1, 2, 3, 4, 5, 6, 10};
  tp.for_each(data.begin(), data.end(), [](int i) {
    std::cout << "factorial(" << i << ") = " << factorial(i) << std::endl;
  });

  tp.shutdown();
  std::cerr << "main exit.." << std::endl;

  return 0;
}
