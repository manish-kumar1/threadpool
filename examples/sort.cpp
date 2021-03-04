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

#include "include/threadpool.hpp"
#include "include/clock_util.hpp"

using namespace std;
using namespace thp;
using namespace chrono;

template<typename Data>
void sort_thp(threadpool& tp, Data& data) {
  tp.sort(data.begin(), data.end());
  tp.drain();
}


int main(int argc, const char* const argv[]) {
  util::clock_util<chrono::steady_clock> cp;
  const unsigned  times = 10*1000000; // 10 million
  auto N = argc > 1 ? stoi(argv[1]) : times;
  bool use_stl = false;

  std::locale::global(std::locale(""));
  std::cerr.imbue(std::locale(""));

  try {
    thp::threadpool tp;
    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<int> dis(numeric_limits<int>::min()+1, numeric_limits<int>::max()-1);
    //std::uniform_int_distribution<char> dis('a', 'z');

    std::cerr << "size          time(ms)    is_sorted     stl(ms)" << std::endl;
    for(unsigned n = 10; n <= N; n *= 10) {
      std::vector<int> data;
      data.reserve(n);

      std::generate_n(std::back_inserter(data), n, [&] { return dis(e); });

      cp.now();
      sort_thp(tp, data);
      cp.now();

      std::cerr << std::left << std::setw(14) << data.size() << std::setw(8) << cp.get_ms() << std::right << std::setw(10) << std::ranges::is_sorted(data);

      if (use_stl) {
        std::generate_n(data.begin(), n, [&] { return dis(e); });
        cp.now();
        std::sort(data.begin(), data.end());
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
