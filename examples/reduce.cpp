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
  //using pf = thp::part_algo<decltype(data.begin())>;
  std::size_t step = std::clamp(static_cast<unsigned>(data.size()/std::thread::hardware_concurrency()), 100000u, 25000000u); //250000; // tune it for your data size
  if (step == 0) step = 1;

  thp::EqualSizePartAlgo algo(data.cbegin(), data.cend(), step);

  auto [f] = tp.reduce(data.cbegin(), data.cend(),
                       std::numeric_limits<int>::max(),  // T
                       [](auto&& a, auto&& b) { return std::min(a, b); }, // Binary op
                       algo // partitioner
                     );

  return f.get();    
}

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  //google::InitGoogleLogging(argv[0]);
  
  const unsigned n = argc > 1 ? std::stoi(argv[1]) : 10*1000000; // 10 million
  bool use_stl = argc > 2;;

  std::locale::global(std::locale(""));
  std::cerr.imbue(std::locale(""));

  try {
    std::random_device r;
    std::default_random_engine engine(r());
    std::uniform_int_distribution<int> dis(-42, std::numeric_limits<int>::max());
    thp::util::clock_util<std::chrono::system_clock> cu;
    std::vector<int> data;
    data.reserve(n);
    std::generate_n(std::back_inserter(data), n, [&] { return dis(engine); });
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
      std::cerr << "stl(" << data.size() << "): " << cu.get_ms() << " ms" << ", ans = " << smin << std::endl;
    }
    else
    {
      cu.now();
      thp::threadpool tp;
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

