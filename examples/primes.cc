#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <glog/logging.h>
#include <vector>
#include <map>
#include <ranges>
#include <cmath>
#include <iterator>
#include <sstream>

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
  //auto x = s == 0 ? 0.01*(e-s) : std::ceil((e-s)/std::log(s));
  //ret.reserve(x);
  ret.reserve(static_cast<unsigned>(0.01*(e-s)));
  for (unsigned i = s; i < e; ++i)
    if (is_prime(i)) ret.push_back(i);

  return ret;
}

auto seq_prime(unsigned int N) {
    namespace rng = std::ranges;
    std::vector<decltype(N)> prime_vec;
    prime_vec.reserve(0.075*N);
    std::promise<std::vector<unsigned int>> ans;
    std::vector<std::future<std::vector<unsigned int>>> futs;

    auto completely_divides = [](const auto pp) { return [pp](auto x) { /*std::cerr << pp << "%" << x << " == " << pp%x << "\n"; */return pp%x == 0; }; };
    auto primes = [&] (auto n) {
        //std::cerr << std::addressof(prime_vec) << ", " << prime_vec.size() << std::endl;
        auto factors = prime_vec | views::take_while([sq = static_cast<unsigned>(sqrt(n))+1](auto&& p) { return p < sq; });
        auto yes = rng::none_of(factors , completely_divides(n));
        if (yes) prime_vec.push_back(n);
        return yes;
    };
    std::ostream ss(nullptr);
    auto prime_nums = views::iota(2u, N+1) | views::filter(primes);
    rng::copy(prime_nums, std::ostream_iterator<decltype(N)>(ss, ""));
    //std::cerr << "prime nums: " << prime_vec.size() << std::endl;
    ans.set_value(std::move(prime_vec));
    futs.emplace_back(ans.get_future());
    return futs;
}

auto check_prime(thp::threadpool* tp, const unsigned int n = 10*1000000) {
  std::vector<std::shared_ptr<simple_task<std::vector<unsigned>>>> tasks;
  auto step = std::clamp(n/std::thread::hardware_concurrency(), 50000u, 1000000u);
  tasks.reserve(std::ceil(n/step));
  util::clock_util<chrono::steady_clock> cp;
  for (unsigned int i = 0; i < n; i += step) {
      tasks.emplace_back(make_task(find_prime, i, std::min(i+step, n)));
  }

  auto nts = tasks.size();
  cp.now();
  auto [ret] = tp->schedule(std::move(tasks));
  tp->drain();
  cp.now();
  std::cerr << "time = " << cp.get_ms() << ", " << step << ", " << nts << std::endl;
  return std::move(ret);
}

int main(int argc, const char* const argv[]) {
  auto N = argc > 1 ? stoi(argv[1]) : 10*1000000;
  util::clock_util<chrono::steady_clock> cp;

  //std::locale::global(std::locale(""));
  //std::cerr.imbue(std::locale("")); 
 
  try {
    thp::threadpool tp;
    cp.now();
    auto primes = check_prime(&tp, N);
    //auto primes = seq_prime(N);
    cp.now();

    unsigned num_primes = 0;
    std::ranges::for_each(primes, [&](auto&& f) {
         auto&& v = f.get();
         num_primes += v.size();
         //std::ranges::copy(v, std::ostream_iterator<unsigned>(std::cerr, "\n"));
         //std::copy_n(v.begin(), 10, std::ostream_iterator<unsigned>(std::cerr, "\n"));
    });
    
    std::cerr << "total primes: (" << N << ") : " << num_primes << std::endl;
    std::cerr << "thp: " << cp.get_ms() << " ms" << std::endl;
  } catch (exception &ex) {
    cerr << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    cerr << "Exception: " << endl;
  }

  return 0;
}
