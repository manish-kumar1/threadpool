#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <glog/logging.h>
#include <vector>
#include <map>

#include "include/threadpool.hpp"
#include "include/clock_util.hpp"

using namespace std;
using namespace thp;
using namespace chrono;

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

auto find_prime(unsigned s, unsigned e) {
  std::vector<unsigned> ret;
  for (unsigned i = s; i < e; ++i)
    if (is_prime(i)) ret.push_back(i);

  return ret;
}

auto check_prime(thp::threadpool* tp) {
  const unsigned int times = 10000000; // 10 million
  std::vector<std::future<std::vector<unsigned>>> ret;
  auto step = 1000000;
  for (unsigned int i = 0; i < times; i += step) {
      auto [fut] = tp->enqueue(find_prime, i, i+step);
      ret.emplace_back(std::move(fut));
  }
  std::cerr << "check_prime created" << std::endl;
  return ret;
}

int main(int argc, const char* const argv[]) {
//  google::InitGoogleLogging(argv[0]);
  util::clock_util<chrono::system_clock> cp;
  try {
    cp.now();
    thp::threadpool tp;
    auto primes = check_prime(&tp);
    tp.drain();
    cp.now();

    for(auto& f : primes)
      for(auto&& v : f.get())
        std::cout << v << "\n";

    std::cerr << "thp: " << cp.get_ms() << " ms" << std::endl;
  } catch (exception &ex) {
    cerr << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    cerr << "Exception: " << endl;
  }

  cerr << "main exiting" << endl;
  return 0;
}
