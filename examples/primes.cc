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
#include <bitset>
#include <cfenv>

#include "include/threadpool.hpp"
#include "include/clock_util.hpp"

using namespace std;
using namespace thp;
using namespace chrono;

template<std::size_t N>
struct PrimeBits {
    uint64_t start;
    std::bitset<N> bits;
};

bool is_prime(const unsigned n, const std::vector<unsigned>& factors, std::vector<unsigned>::const_iterator& re) {
  bool ret = true;
  auto sq = static_cast<unsigned>(1 + sqrt(n));
  if (sq <= factors.back()) {
    ret = std::ranges::none_of(factors.begin(), re, [n](unsigned x) { return n%x == 0; });
  }
  else
  {
    if (std::ranges::none_of(factors, [n](unsigned x) { return n%x == 0; })) {
      for (unsigned i = 2+factors.back(); i < sq; i += 2) {
        if (n % i == 0) {
          ret = false;
          break;
        }
      }
    }
    else
      ret = false;
  }
  return ret;
}

auto find_prime(const std::vector<unsigned>& factors, unsigned s, unsigned e) {
  std::vector<unsigned> primes;
  primes.reserve(std::ceil(0.070435*(e-s)));
  auto sq = static_cast<unsigned>(1 + sqrt(e));
  auto re = std::ranges::lower_bound(factors, sq);
for (unsigned n = s; n < e; ++n) {
  bool ret = true;
  if (sq <= factors.back()) {
    ret = std::ranges::none_of(factors.begin(), re, [n](unsigned x) { return n%x == 0; });
  }
  else
  {
    if (std::ranges::none_of(factors, [n](unsigned x) { return n%x == 0; })) {
      for (unsigned i = 2+factors.back(); i < sq; i += 2) {
        if (n % i == 0) {
          ret = false;
          break;
        }
      }
    }
    else
      ret = false;
  }
  if (ret) primes.push_back(n);
}
  //std::cerr << "[" << s << ", " << e << ") = " << primes.size() << std::endl;
  return primes;
}

auto seq_prime(unsigned int N) {
    namespace rng = std::ranges;
    std::vector<decltype(N)> prime_vec;
    prime_vec.reserve(0.070435*N);

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
    return prime_vec;
}

#if 0
template<std::size_t N>
auto generate_seq_prime() {
    namespace rng = std::ranges;
    std::bitset<N> ans;
    std::vector<decltype(N)> prime_vec;
    prime_vec.reserve(0.075*N);

    auto completely_divides = [](const auto pp) { return [pp] (auto x) { /*std::cerr << pp << "%" << x << " == " << pp%x << "\n"; */return pp%x == 0; }; };
    auto primes = [&] (auto n) {
        auto factors = prime_vec | views::take_while([sq = static_cast<unsigned>(sqrt(n))+1](auto&& p) { return p < sq; });
        auto yes = rng::none_of(factors , completely_divides(n));
        if (yes) prime_vec.push_back(n);
        return yes;
    };
    auto prime_nums = views::iota(2u, N+1) | views::filter(primes);
    rng::for_each(prime_nums, [&ans](auto&& n) { ans.set(n); });
    return ans;
}


template<std::integral T>
decltype(auto) list_prime(thp::threadpool* tp, T n) {
  namespace rng = std::ranges;
  constexpr T interval_len = 1024*1024;
  sync_container<std::shared_ptr<ans<interval_len>>> primes(std::ceil(n/interval_len));

  primes.put(std::make_shared<ans<interval_len>>(T{0}, generate_seq_prime<interval_len>()));

  auto find_prime = [&](const T start, const T end) {
    //std::cerr << "find_prime(" << start << "," << end << ")" << std::endl;
    const auto sq = std::rint(1+sqrt(end));
    const auto idx = std::rint(0.5 + double(sq)/interval_len);

    auto ret = std::make_shared<ans<interval_len>>();
    ret->start = start;

    for(T x = start; x < end; ++x) {
      for(auto j = 0; j < idx; ++j) {
      auto& prime_vec = primes[j];
        ret->prime_bits[x - start] = true;
        for(auto off = 0; off+ret->start < sq; ++off) {
          if (prime_vec->prime_bits[off]) {
          if (x % factor == 0) {
            ret->prime_bits[x - start] = false;
            break;
          }
        }
      }
    }

    //std::cerr << "find_prime(" << start << "," << end << ") = " << ret->start << "+" << ret->prime_bits.count() << std::endl;
    //primes.put(std::move(ret));
    return ret;
  };

  std::vector<std::unique_ptr<simple_task<std::shared_ptr<ans<interval_len>>>>> tasks;
  auto step = interval_len;

  for(T i = step; i < n; i += step)
    tasks.emplace_back(make_task(find_prime, i, std::min(i+step, n)));

  auto [futs] = tp->schedule(std::move(tasks));
  rng::for_each(futs, [&](auto&& f) { primes.put(std::move(f.get())); });
  unsigned tot = 0;
  for(int i = 0; i < primes.size(); ++i) {
    auto& p = primes[i];
    tot += p->prime_bits.count();
    //for(auto j = p->start, off = 0; off < p->prime_bits.size(); ++off) {
    //  if (p->prime_bits[off]) std::cerr << p->start+off << std::endl;
    //}
  };
  return tot;
}
#endif

auto check_prime(thp::threadpool* tp, const unsigned int n = 10*1000000) {
  std::vector<std::unique_ptr<simple_task<std::vector<unsigned>>>> tasks;
  auto step = std::clamp(n/std::thread::hardware_concurrency(), 32*1024u, 256*10240u);
  tasks.reserve(std::ceil(double(n)/step));
  const auto sieve = seq_prime(step);
  for (unsigned int i = step; i < n; i += step) {
      tasks.emplace_back(make_task(find_prime, std::cref(sieve), i, std::min(i+step, n)));
  }

  auto [ret] = tp->schedule(std::move(tasks));
  tp->drain();
  //std::ranges::for_each(ret, [](auto&& f) { f.wait(); });
  //std::cerr << "time = " << cp.get_ms() << ", " << step << ", " << nts << std::endl;
  return std::move(ret);
}

int main(int argc, const char* const argv[]) {
  auto N = argc > 1 ? stoi(argv[1]) : 10*1000000;
  util::clock_util<chrono::steady_clock> cp;

  auto save_rounding = std::fegetround();
  std::fesetround(FE_UPWARD);

  //auto lcl = std::locale("");
  //std::locale::global(lcl);
  //std::cerr.imbue(lcl); 
 
  try {
    thp::threadpool tp;
    cp.now();
    auto primes = check_prime(&tp, N);
    //auto primes = seq_prime(N);
    //auto tot = list_prime(&tp, N);
    cp.now();

    unsigned num_primes = 0;
//#if 0
    std::ranges::for_each(primes, [&](auto&& f) {
         auto&& v = f.get();
         num_primes += v.size();
         //std::ranges::copy(v, std::ostream_iterator<unsigned>(std::cerr, "\n"));
         //std::copy_n(v.begin(), 10, std::ostream_iterator<unsigned>(std::cerr, "\n"));
    });
//#endif    
    std::cerr << "total primes: (" << N << ") : " << num_primes << std::endl;
    std::cerr << "thp: " << cp.get_ms() << " ms" << std::endl;
  } catch (exception &ex) {
    cerr << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    cerr << "Exception: " << endl;
  }

  std::fesetround(save_rounding);

  return 0;
}
