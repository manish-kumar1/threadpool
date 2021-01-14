#ifndef CLOCK_UTIL_HPP
#define CLOCK_UTIL_HPP

#include <chrono>
#include <ratio>

namespace thp {
namespace util {

template <typename Clock>
class clock_util {
public:
  constexpr explicit clock_util() : s {Clock::time_point::min()}, e{Clock::time_point::min()} {}

  constexpr decltype(auto) now() noexcept {
    tmp = Clock::now();
    std::swap(s, e);
    std::swap(tmp, e);
    return *this;
  }

  constexpr double get_us() noexcept {
    std::chrono::duration<double, std::micro> tot = e-s;
    return tot.count();
  }

  constexpr double get_ms() noexcept {
    return get_us()/1000.0f;
  }

  constexpr double get_sec() noexcept {
    return get_ms()/1000.0f;
  }

private:
  typename Clock::time_point s, e, tmp;
};

} // namespace util
} // namespace thp

#endif // CLOCK_UTIL_HPP
