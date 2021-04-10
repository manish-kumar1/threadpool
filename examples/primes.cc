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
      T i = p*p;
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
  auto less_than = [](const auto pp) { return [pp] (auto x) { return x < pp; }; };
  auto non_prime_bits = [&] (auto&& s) { return [s,&primes] (auto&& n) { return primes.test(n - s); }; };
  auto update_prime_bits = [&](auto&& x) {
    primes.update_prime_bits(x);
    return primes.test(x-start);
  };

  // find primes for first segment
  rng::for_each(small_primes | views::take_while(less_than(sq)), [&](auto&& p) { primes.update_prime_bits(p); });

  auto all_primes_bits = views::iota(start, end) | views::filter(non_prime_bits(start)) | views::transform(update_prime_bits);
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
  constexpr const size_t N = 16*8*1024u;
  using ReturnType = PrimeBits<T, N>;
  auto step = N; 
  std::vector<T> sieve;

  const auto smalls = seq_prime<T>(static_cast<T>(1 + sqrt(step)));
  const auto sieve_bits = find_prime<T, N>(smalls, 2, step);
  sieve_bits.print(std::back_inserter(sieve));
  //sieve_bits.print(std::ostream_iterator<T>(std::cout, ", "));

  std::promise<ReturnType> first;
  first.set_value(std::move(sieve_bits));

  std::vector<simple_task<ReturnType>> tasks;
  tasks.reserve(std::ceil(n/step));
  for (T i = step; i < n; i += step) {
    tasks.emplace_back(make_task(find_prime<T, N>, sieve, std::forward<const T>(i), std::forward<const T>(std::min(i+step, n))));
  }

  //auto work = job<simple_task<ReturnType>>(std::move(tasks)).num_workers(16);
  //auto futs = tp->run(work);
  auto [futs] = tp->schedule(std::move(tasks));
  futs.emplace_front(first.get_future());

  return std::move(futs);
}

int main(int argc, const char* const argv[]) {
  auto n = argc > 1 ? stoull(argv[1]) : 10*1000000u;
  auto workers = argc > 2 ? stoi(argv[2]) : std::thread::hardware_concurrency();
  
  util::clock_util<chrono::high_resolution_clock> cp;
  
  std::ios::sync_with_stdio(false);
  std::ostream& oss = std::cout;
  if (std::cin.tie()) std::cin.tie(nullptr);

  auto save_rounding = std::fegetround();
  std::fesetround(FE_UPWARD);

  auto lcl = std::locale("");
  std::locale::global(lcl);
  oss.imbue(lcl); 
 
  try {
    thp::threadpool tp(workers);
    cp.now();
    auto primes = check_prime(&tp, n);
    //auto primes = seq_prime(n);

    unsigned num_primes = 0;
    std::ranges::for_each(primes, [&](auto&& f) {
      auto&& v = f.get();
      num_primes += v.count();
  //    v.print(std::ostream_iterator<decltype(n)>(oss, "\n"));
    });
    cp.now();
    oss << "total primes: (" << n << ") : " << num_primes << "\n";
    oss << "thp: " << cp.get_ms() << " ms" << std::endl;
  } catch (exception &ex) {
    oss << "main: Exception: " << ex.what() << endl;
  } catch (...) {
    oss << "Exception: " << endl;
  }

  std::fesetround(save_rounding);

  return 0;
}
