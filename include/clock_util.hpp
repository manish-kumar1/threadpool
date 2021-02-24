#ifndef CLOCK_UTIL_HPP
#define CLOCK_UTIL_HPP

#include <chrono>
#include <ratio>

namespace thp {
namespace util {

template <typename Clock>
class clock_util {
public:
  constexpr explicit clock_util() : s {Clock::now()}, e{Clock::now()} {}

  constexpr decltype(auto) now() noexcept {
    tmp = Clock::now();
    std::swap(s, e);
    std::swap(tmp, e);
    return *this;
  }

  template<typename T, typename C>
  inline constexpr std::chrono::duration<T, C> last_dur() const {
    return (e - s);
  }

  constexpr double get_us() noexcept {
    std::chrono::duration<double, std::micro> tot = e-s;
    return tot.count();
  }

  constexpr double get_ms() noexcept {
    std::chrono::duration<double, std::milli> tot = e-s;
    return tot.count();
    //return get_us()/1000.0f;
  }

  constexpr double get_sec() noexcept {
    //std::chrono::duration<double, std::chrono::seconds> tot = e-s;
    //return tot.count();
    return get_ms()/1000.0f;
  }

private:
  typename Clock::time_point s, e, tmp;
};

} // namespace util
} // namespace thp

#endif // CLOCK_UTIL_HPP
