#include <iostream>
#include <string>
#include <random>
#include <tuple>
#include <chrono>
#include <cassert>
#include <execution>
#include <glog/logging.h>

#include "include/threadpool.hpp"
#include "include/partitioner.hpp"
#include "include/clock_util.hpp"


decltype(auto) reduce_min(const std::vector<int>& data, thp::threadpool& tp) {
  using pf = thp::part_algo<decltype(data.begin())>;
  pf algo(data.begin(), data.end());
  algo.step = 550000; // tune it for your data size

  auto [f] = tp.reduce(data.begin(), data.end(),
                       std::numeric_limits<int>::max(),  // T
                       [](auto&& a, auto&& b) { return std::min(a, b); }, // Binary op
                       thp::partitioner(data.begin(), data.end(), algo) // partitioner
                     );

  return f.get();    
}

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  try {
    std::random_device r;
    std::default_random_engine engine(r());
    std::uniform_int_distribution<int> dis(-100, std::numeric_limits<int>::max());
    constexpr const unsigned n = 40*1000000; // 40 million
    std::vector<int> data;
    data.reserve(n);
    std::generate_n(std::back_inserter(data), n, [&] { return dis(engine); });
    thp::util::clock_util<std::chrono::system_clock> cu;
    {
      cu.now();
      auto smin = std::transform_reduce(std::execution::par,
                                      data.cbegin(), data.cend(),
                                      std::numeric_limits<int>::max(),
                                      [](auto&& a, auto&& b) { return std::min(a, b); },
                                      std::identity()
                                      );
      cu.now();
      std::cerr << "stl(" << data.size() << "): " << cu.get_ms() << " ms" << ", ans = " << smin << std::endl;
    }
    {
      thp::threadpool tp;
      cu.now();
      auto tmin = reduce_min(data, std::ref(tp));    
      cu.now();
      //std::cerr << std::format("thp({}): {} ms, ans = {}\n", data.size(), cu.get_ms(), tmin);
      std::cerr << "thp(" << data.size() << "): " << cu.get_ms() << " ms" << ", ans = " << tmin << std::endl;
      tp.shutdown();
    }
  } catch(std::exception& ex) {
    std::cerr << "main: Exception: " << ex.what() << std::endl;
  } catch(...) {
    std::cerr << "Exception: " << std::endl;
  }
  return 0;
}

