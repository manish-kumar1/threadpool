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
namespace rng = std::ranges;

template<std::integral T>
struct PrimeBits {
    explicit PrimeBits(T s, std::size_t knts)
    : start{s}
    , total{0}
    , bits(knts, true) {
    }
    T start, total;
    std::vector<bool> bits;
    decltype(auto) size() { return total; }
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

template<std::integral T>
PrimeBits<T> find_prime(const std::vector<T>& factors, const T start, const T end) {
  //auto knts = std::ceil(1.25506f*(double(end)/std::log(end) - double(start)/std::log(start)));
  PrimeBits<T> primes{start, end-start};

  const auto sq = static_cast<T>(1 + sqrt(end));
  const auto re = std::ranges::lower_bound(factors, sq);
  const auto fe = std::ranges::lower_bound(factors, (end-start)/2);

  auto completely_divides = [](const auto pp) { return [pp](auto x) { return pp%x == 0; }; };
  auto fill_bits = [&](const T p) {
    //std::cerr << "fill_bits: " << p << ", " << start << ", " << end << std::endl;
    T i = p;
    if (start > p) {
      auto d = start/p;
      i += d*p;
    } else {
      i += p;
      primes.bits[p-start] = true;
    }

    for (; i < end; i += p) {
      primes.bits[i-start] = false;
    }
  };

  rng::for_each(factors.begin(), fe, fill_bits);

  for (T n = start; n < end; ++n) {
    if (primes.bits[n - start]) {
      bool yes = true;
      if (sq <= factors.back()) {
        yes = std::ranges::none_of(factors.begin(), re, [n](auto&& x) { return n%x == 0; });
      }
      else
      {
        if (std::ranges::none_of(factors, [n](auto&& x) { return n%x == 0; })) {
          for (auto i = factors.back()+2; i < sq; i += 2) {
            if (n % i == 0) {
              yes = false;
              break;
            }
          }
        }
        else
          yes = false;
      }
      if (yes) {
        fill_bits(n);
        primes.bits[n - start] = true;
        ++primes.total;
      } else {
        primes.bits[n - start] = false;
      }
    }
  }
  //std::cerr << "[" << start << ", " << end << ") = " << primes.size() << ", "  << knts << ", " << (primes.size()*16) << ", " << (end-start) << std::endl;
  return primes;
}

template<std::integral T>
auto seq_prime(const T N) {
    namespace rng = std::ranges;
    std::vector<T> prime_vec;
    prime_vec.reserve(0.070435*N);

    auto completely_divides = [](const auto pp) { return [pp](auto x) { return pp%x == 0; }; };
    auto primes = [&] (auto n) {
        //std::cerr << std::addressof(prime_vec) << ", " << prime_vec.size() << std::endl;
        auto factors = prime_vec | views::take_while([sq = static_cast<T>(sqrt(n))+1](auto&& p) { return p < sq; });
        auto yes = rng::none_of(factors , completely_divides(n));
        if (yes) prime_vec.push_back(n);
        return yes;
    };
    std::ostream ss(nullptr);
    auto prime_nums = views::iota(2u, N+1) | views::filter(primes);
    rng::copy(prime_nums, std::ostream_iterator<T>(ss, ""));
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

template<std::integral T>
auto check_prime(thp::threadpool* tp, const T n = 10*1000000) {
  T start_offset = 16*8*1024u;
  T end_offset = 16*8*1024u;

  std::vector<std::unique_ptr<simple_task<PrimeBits<T>>>> tasks;
  auto step = std::clamp(n/std::thread::hardware_concurrency(), start_offset, end_offset); //65536u, 65536u); //32*1024u, 256*10240u); // 783500
  tasks.reserve(std::ceil(n/step));

  const auto sieve = seq_prime<T>(step);

  std::promise<PrimeBits<T>> first;
  PrimeBits<T> nums(0, step);
  nums.start = 0;
  nums.total = sieve.size();
  nums.bits.flip();
  rng::for_each(sieve, [&](auto&& p) { nums.bits.at(p) = true; });

  first.set_value(std::move(nums));

  for (T i = step; i < n; i += step) {
      tasks.emplace_back(make_task(find_prime<T>, sieve, std::forward<const T>(i), std::forward<const T>(std::min(i+step, n))));
  }

  auto [ret] = tp->schedule(std::move(tasks));

  std::vector<std::future<PrimeBits<T>>> ans;
  ans.reserve(ret.size()+1);

  ans.emplace_back(first.get_future());

  rng::move(ret, std::back_inserter(ans));

  return std::move(ans);
}

int main(int argc, const char* const argv[]) {
  auto N = argc > 1 ? stoull(argv[1]) : 10*1000000;
  util::clock_util<chrono::steady_clock> cp;

  auto save_rounding = std::fegetround();
  std::fesetround(FE_UPWARD);

  auto lcl = std::locale("");
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
      //std::cerr << "size: " << v.size() << std::endl;
//      auto i = v.start;
//      for(auto&& b : v.bits) {
//        if (b) std::cerr << i << '\n';
//        ++i;
//      }
      //rng::transform(v.off, std::ostream_iterator<uint64_t>(std::cerr, "\n"), [&](auto&& o) { return v.start + o; });
      //std::cout << "---------" << std::endl;
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
