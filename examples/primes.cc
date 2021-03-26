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
struct PrimeBits {
    explicit PrimeBits(T s, T e)
    : start{s}
    , total{0}
    , bits(e-s, true) {
    }
    T start;
    T total;
    std::vector<bool> bits;

    decltype(auto) count() { return total; }

    decltype(auto) print(std::ostream& oss, const std::string sep = "\n") {
      auto i = start;
      for(auto&& b : bits) {
        if (b)
          oss << i << sep.c_str();
        ++i;
      }
      return oss;
    }

    decltype(auto) update_prime_bits(const T p) {
      //std::cout << "[" << start << " " << end << ") p = " << p << " x " << std::endl;
      T i = start;
      const T end = start+bits.size();
      if (start > p) {
        if (start % p == 0)
          i = start;
        else
          i = p*(1 + start/p);
      }
      else
        i = 2*p;

      for(; i < end; i += p)
        bits[i-start] = false;
    }
};

template<std::integral T>
PrimeBits<T> find_prime(const std::vector<T>& small_primes, const T start, const T end) {
  //auto knts = std::ceil(1.25506f*(double(end)/std::log(end) - double(start)/std::log(start)));
  PrimeBits<T> primes{start, end};

  const auto sq = static_cast<T>(1 + sqrt(end));
  auto completely_divides = [](const auto pp) { return [pp](auto x) { return pp%x == 0; }; };
  auto less_than = [](const auto pp) { return [pp] (auto x) { return x < pp; }; };
  auto non_prime_bits = [&] (auto&& s) { return [s,&primes] (auto&& n) { return primes.bits[n - s]; }; };
  auto new_primes = [&](auto&& n) {
    auto divides = completely_divides(n);
    auto factors = small_primes | views::take_while(less_than(sq));
    bool yes = rng::none_of(factors, divides);
    if (sq > small_primes.back()) {
      for(auto i = small_primes.back()+2; i < sq; i += 2)
        if (divides(i)) {
          yes = false;
          break;
        }
      }
    return yes;
  };
  auto update_prime_bits = [&](auto&& x) {
    primes.update_prime_bits(x);
    return primes.bits[x-start];
  };

  auto all_primes_bits = views::iota(start, end) | views::filter(non_prime_bits(start)) | views::transform(update_prime_bits);

  rng::for_each(small_primes | views::take_while(less_than(sq)), [&](auto&& p) { primes.update_prime_bits(p); });
  primes.total = rng::count(all_primes_bits, true);

  if (primes.total != primes.count()) std::cerr << "bug " << primes.total << " != " << primes.count() << std::endl;
  //std::cerr << "[" << start << ", " << end << ") = " << primes.size() << ", "  << sq << ", " << (primes.size()*16) << ", " << (end-start) << std::endl;
  return primes;
}

template<std::integral T>
auto seq_prime(const T N) {
    namespace rng = std::ranges;
    std::vector<T> prime_vec;

    auto completely_divides = [](const auto pp) { return [pp](auto x) { return pp%x == 0; }; };
    auto primes = [&] (const T n) {
        auto factors = prime_vec | views::take_while([sq = static_cast<T>(sqrt(n))+1](auto&& p) { return p < sq; });
        auto yes = rng::none_of(factors , completely_divides(n));
        return yes;
    };
    auto prime_nums = views::iota(2u, N+1) | views::filter(primes);
    rng::copy(prime_nums, std::back_inserter(prime_vec));
    return prime_vec;
}

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

  return ans;
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
    std::ranges::for_each(primes, [&](auto&& f) {
      auto&& v = f.get();
      num_primes += v.count();
      //v.print(std::cerr);
    });
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
