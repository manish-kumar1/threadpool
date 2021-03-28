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

template<std::integral T, std::size_t N>
struct PrimeBits {
  T start, total;
  std::bitset<N> bits;

  template<typename V>
  struct iterator {
    V start;
    std::size_t pos;
    const std::bitset<N>& data;

    constexpr explicit iterator(const std::bitset<N>& d, V s, std::size_t p = 0)
    : start{s}
    , pos{p}
    , data{d}
    {}

    constexpr iterator& operator ++() { ++pos; return *this; }
    constexpr iterator& operator++(std::size_t) { pos++; return *this; }
    constexpr decltype(auto) operator * () { return data[pos]; }
    friend bool operator == (iterator a, iterator b) {
      return (a.pos == b.pos) && (std::addressof(a.data) == std::addressof(b.data)) && (a.start == b.start);
    }
  };

public:
    constexpr explicit PrimeBits(T s, T e)
    : start{s}
    , total{0}
    , bits()
    {
      bits.set();
      while(e-s < N) {
        bits.set(e-s, false);
        ++e;
      }
    }

    constexpr decltype(auto) count() const noexcept { return bits.count(); }

    template<typename OIter>
    constexpr decltype(auto) print(OIter iter) const {
      auto val = views::iota(start, start+N) | views::filter([&](auto&& x) { return test(x-start); });
      return rng::copy(val, iter);
    }

    constexpr decltype(auto) update_prime_bits(const T p) {
      //std::cout << "[" << start << " " << end << ") p = " << p << " x " << std::endl;
      T i = 2*p;
      if (start > p) {
        i = (start % p == 0) ? start : p*(1 + start/p);
      }

      for(auto j = i-start; j < N; j += p) {
        //bits.set(j, false);
        bits[j] = false;
      }
    }

    constexpr decltype(auto) set(std::size_t pos)        { return bits.set(pos);  }
    constexpr decltype(auto) test(std::size_t pos) const { return bits.test(pos); }
};

template<std::integral T, std::size_t N>
constexpr PrimeBits<T, N> find_prime(const std::vector<T>& small_primes, const T start, const T end) {
  //auto knts = std::ceil(1.25506f*(double(end)/std::log(end) - double(start)/std::log(start)));

  PrimeBits<T, N> primes{start, end};

  const auto sq = static_cast<T>(1 + sqrt(end));
  auto completely_divides = [](const auto pp) { return [pp](auto x) { return pp%x == 0; }; };
  auto less_than = [](const auto pp) { return [pp] (auto x) { return x < pp; }; };
  auto non_prime_bits = [&] (auto&& s) { return [s,&primes] (auto&& n) { return primes.test(n - s); }; };
  auto update_prime_bits = [&](auto&& x) {
    primes.update_prime_bits(x);
    return primes.test(x-start);
  };

  auto all_primes_bits = views::iota(start, end) | views::filter(non_prime_bits(start)) | views::transform(update_prime_bits);

  // find primes for first segment
  rng::for_each(small_primes | views::take_while(less_than(sq)), [&](auto&& p) { primes.update_prime_bits(p); });
  primes.total = rng::count(all_primes_bits, true);

  //std::cerr << "[" << start << ", " << end << ") = " << primes.size() << ", "  << sq << ", " << (primes.size()*16) << ", " << (end-start) << std::endl;
  return primes;
}

template<std::integral T>
constexpr auto seq_prime(const T N) {
    namespace rng = std::ranges;
    std::vector<T> prime_vec;
    //prime_vec.reserve(std::ceil((1.25506f*N)/std::log(N)));

    auto completely_divides = [](const auto pp) { return [pp](auto x) { return pp%x == 0; }; };
    auto primes = [&] (const T n) {
        auto factors = prime_vec | views::take_while([sq = static_cast<T>(sqrt(n))+1](auto&& p) { return p < sq; });
        return rng::none_of(factors , completely_divides(n));
    };
    auto prime_nums = views::iota(2u, N+1) | views::filter(primes);
    rng::copy(prime_nums, std::back_inserter(prime_vec));
    return prime_vec;
}

template<std::integral T>
auto check_prime(thp::threadpool* tp, const T n = 10*1000000) {
  constexpr const T start_offset = 16*8*1024u;
  constexpr const T end_offset = 16*8*1024u;
  constexpr const size_t N = start_offset;
  std::vector<simple_task<PrimeBits<T, N>>> tasks;
  auto step = N; 
  tasks.reserve(std::ceil(n/step));

  const auto sieve = seq_prime<T>(step);

  std::promise<PrimeBits<T, N>> first;
  PrimeBits<T, N> nums(0, step);
  nums.bits.flip();
  rng::for_each(sieve, [&](auto&& p) { nums.set(p); });

  first.set_value(std::move(nums));

  for (T i = step; i < n; i += step) {
      tasks.emplace_back(make_task(find_prime<T, N>, sieve, std::forward<const T>(i), std::forward<const T>(std::min(i+step, n))));
  }

  auto [ret] = tp->schedule(std::move(tasks));

  std::vector<std::future<PrimeBits<T, N>>> ans;
  ans.reserve(ret.size()+1);

  ans.emplace_back(first.get_future());
  rng::move(ret, std::back_inserter(ans));

  return ans;
}

int main(int argc, const char* const argv[]) {
  auto N = argc > 1 ? stoull(argv[1]) : 10*1000000u;
  util::clock_util<chrono::steady_clock> cp;

  std::ios::sync_with_stdio(false);
  std::ostream& oss = std::cout;

  auto save_rounding = std::fegetround();
  std::fesetround(FE_UPWARD);

  auto lcl = std::locale("");
//  std::locale::global(lcl);
//  oss.imbue(lcl); 
 
  try {
    thp::threadpool tp;
    cp.now();
    auto primes = check_prime(&tp, N);
    //auto primes = seq_prime(N);
    //auto tot = list_prime(&tp, N);

    unsigned num_primes = 0;
    std::ranges::for_each(primes, [&](auto&& f) {
      auto&& v = f.get();
      num_primes += v.count();
      v.print(std::ostream_iterator<decltype(N)>(oss, "\n"));
    });
    cp.now();
    oss << "total primes: (" << N << ") : " << num_primes << std::endl;
    oss << "thp: " << cp.get_ms() << " ms" << std::endl;
  } catch (exception &ex) {
    oss << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    oss << "Exception: " << endl;
  }

  std::fesetround(save_rounding);

  return 0;
}
