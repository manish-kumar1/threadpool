#include <iostream>
#include <string>
#include <random>
#include <tuple>
#include <chrono>
#include <cassert>
#include <execution>
#include <glog/logging.h>
#include <locale>

#include "include/threadpool.hpp"
#include "include/partitioner.hpp"
#include "include/clock_util.hpp"

template<typename Data>
decltype(auto) reduce_min(const Data& data, thp::threadpool& tp) {
  std::size_t num_partitions = std::clamp(data.size()/100000u, std::size_t(1), data.size());
  thp::partition::EqualSize algo(data.cbegin(), data.cend(), num_partitions);

  auto [f] = tp.reduce(data.cbegin(), data.cend(),
                       std::numeric_limits<int>::max(),  // T
                       [](auto&& a, auto&& b) { return std::min(a, b); }, // Binary op
                       algo // partitioner
                     );

  return f.get();    
}

// faster than mt19937 but not very random from stackoverflow
static unsigned long x=123456789, y=362436069, z=521288629;
unsigned long xorshf96(void) {          //period 2^96-1
unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

  return z;
}

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  //google::InitGoogleLogging(argv[0]);
  
  const unsigned n = argc > 1 ? std::stoi(argv[1]) : 10*1000000; // 10 million
  const unsigned workers = argc > 2 ? std::stoi(argv[2]) : std::thread::hardware_concurrency();
  bool use_stl = argc > 3;

  std::locale::global(std::locale(""));
  auto old = std::cout.imbue(std::locale(""));

  thp::util::clock_util<std::chrono::steady_clock> cu;
  try {
    std::random_device r;
    std::mt19937_64 engine(r());
    std::uniform_int_distribution<int> dis(-42, std::numeric_limits<int>::max());
    std::vector<int> data;
    data.reserve(n);
    std::generate_n(std::back_inserter(data), n, [&] { return dis(engine); });
    cu.now();
    std::cout << "data generation (" << data.size() << "): " << cu.get_ms() << " ms\n";
    if (use_stl)
    {
      cu.now();
      auto smin = std::transform_reduce(std::execution::par,
                                      data.cbegin(), data.cend(),
                                      std::numeric_limits<int>::max(),
                                      [](auto&& a, auto&& b) { return std::min(a, b); },
                                      std::identity()
                                      );
      cu.now();
      std::cout << "std::reduce(" << data.size() << "): " << cu.get_ms() << " ms" << ", ans = " << smin << std::endl;
      cu.now();
      smin = std::transform_reduce(
                                      data.cbegin(), data.cend(),
                                      std::numeric_limits<int>::max(),
                                      [](auto&& a, auto&& b) { return std::min(a, b); },
                                      std::identity()
                                      );
      cu.now();
      std::cout << "std::reduce(" << data.size() << "): " << cu.get_ms() << " ms" << ", ans = " << smin << std::endl;
    }
    {
      thp::threadpool tp(workers);
      auto tmin = reduce_min(data, std::ref(tp));    
      cu.now();
      //std::cerr << std::format("thp({}): {} ms, ans = {}\n", data.size(), cu.get_ms(), tmin);
      std::cout << "thp::reduce(" << data.size() << "): " << cu.get_ms() << " ms" << ", ans = " << tmin << std::endl;
    }
  } catch(std::exception& ex) {
    std::cerr << "main: Exception: " << ex.what() << std::endl;
  } catch(...) {
    std::cerr << "Exception: " << std::endl;
  }
  cu.now();
  std::cerr.imbue(old);
  return 0;
}

