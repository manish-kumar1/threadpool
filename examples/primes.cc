#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <glog/logging.h>
#include <vector>
#include <map>
#include <ranges>

#include "include/threadpool.hpp"
#include "include/clock_util.hpp"

using namespace std;
using namespace thp;
using namespace chrono;

// very inefficient prime checker to eat up cpu
bool is_prime(const unsigned n) {
  bool ret = true;
  if (n < 2)
    ret = false;
  else if (n == 2)
    ret = true;
  else if (n%2 == 0)
    ret = false;
  else [[likely]]
  {
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
  ret.reserve(static_cast<unsigned>(0.15*(e-s)));
  for (unsigned i = s; i < e; ++i)
    if (is_prime(i)) ret.push_back(i);

  return ret;
}

auto check_prime(thp::threadpool* tp) {
  const unsigned int times = 10*1000000; // 10 million
  auto step = times/16;
  std::vector<std::shared_ptr<simple_task<std::vector<unsigned>>>> tasks;

  for (unsigned int i = 0; i < times; i += step) {
      tasks.emplace_back(make_task(find_prime, i, std::min(i+step, times)));
  }
  auto [ret] = tp->schedule(std::move(tasks));
  tp->drain();
  return std::move(ret);
}

int main(int argc, const char* const argv[]) {
//  google::InitGoogleLogging(argv[0]);
  //std::ios::sync_with_stdio(false);
  //std::cin.tie(&std::cout);
  util::clock_util<chrono::steady_clock> cp;
  try {
    thp::threadpool tp;
    cp.now();
    auto primes = check_prime(&tp);
    cp.now();

    unsigned num_primes = 0;
    std::ranges::for_each(primes, [&](auto&& f) {
         auto v = f.get();
         num_primes += v.size();
         std::copy_n(v.begin(), 10, std::ostream_iterator<unsigned>(std::cerr, "\n"));
    });
    std::cerr << "total primes: " << num_primes << std::endl;
    std::cerr << "thp: " << cp.get_ms() << " ms" << std::endl;
  } catch (exception &ex) {
    cerr << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    cerr << "Exception: " << endl;
  }

  cerr << "main exiting" << endl;
  return 0;
}
