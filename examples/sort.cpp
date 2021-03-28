#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <glog/logging.h>
#include <vector>
#include <map>
#include <ranges>
#include <locale>
#include <iomanip>
#include <execution>

#include "include/threadpool.hpp"
#include "include/clock_util.hpp"

using namespace std;
using namespace thp;
using namespace chrono;

template<typename T>
void sort_thp(threadpool& tp, T& data) {
  auto [fut] = tp.sort(data.begin(), data.end());
  fut.get();
}

int main(int argc, const char* const argv[]) {
  util::clock_util<chrono::steady_clock> cp;
  const unsigned  times = 10*1000000; // 10 million
  auto N = argc > 1 ? stoi(argv[1]) : times;
  auto workers = argc > 2 ? stoi(argv[2]) : std::thread::hardware_concurrency();
  bool use_stl = argc > 3 ? true : false;

  std::ios::sync_with_stdio(false);
  std::locale l("");
  std::locale::global(l);
  std::cerr.imbue(l);

  try {
    std::random_device r;
    std::default_random_engine e(r());
    //std::uniform_int_distribution<int> dis(numeric_limits<int>::min()+1, numeric_limits<int>::max()-1);
    std::uniform_int_distribution<char> dis('a', 'z');

    thp::threadpool tp(workers);
    std::cerr << std::setw(14) << "size"
              << std::setw(14) << "time (ms)"
              << std::setw(14) << "is_sorted"
              << std::setw(14) << "stl(ms)" << std::endl;
    for(unsigned n = 10; n <= N; n *= 10) {
      std::vector<char> data;
      data.reserve(n);

      std::generate_n(std::back_inserter(data), n, [&] { return dis(e); });

      cp.now();
      auto [fut] = tp.sort(data.begin(), data.end());
      fut.get();
      cp.now();

      std::cerr << std::right << std::setw(14) << data.size()
                << std::setw(14) << cp.get_ms()
                << std::setw(14) << std::ranges::is_sorted(data);

      if (use_stl) {
        std::shuffle(data.begin(), data.end(), e);
        cp.now();
        std::sort(std::execution::par, data.begin(), data.end());
        cp.now();

        std::cerr << std::setw(14) << cp.get_ms();
      }

      std::cerr << std::endl;
      //std::ranges::copy(data, std::ostream_iterator<std::decay_t<decltype(data.at(0))>>(std::cerr, ""));
    }
  } catch (exception &ex) {
    cerr << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    cerr << "Exception: " << endl;
  }

  return 0;
}
