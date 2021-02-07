#include "include/register_types.hpp"
#include "gtest/gtest.h"
#include <tuple>

namespace {

namespace tup_test {
template<std::size_t I, typename T, typename Tup>
constexpr decltype(auto) taskq_idx(Tup& tup) {
  if constexpr (std::is_same_v<T, void>) return 0;
  else if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tup>>)
    return I;
  else
    return taskq_idx<I-1, T, Tup>(tup);
}

TEST(RegisterType, index) {
#define MY_TYPES int, double, float
  std::tuple<MY_TYPES> tup;
//  auto tup = thp::make_task_queues<MY_TYPES>();
  std::size_t N = std::tuple_size<decltype(tup)>::value;

//  std::cout << "for void: " << taskq_for<N, void, ALL_TYPES>(qs) << '\n';
//  std::cout << "for int: " << taskq_for<N, int, ALL_TYPES>(qs) << '\n';
  auto idx = tup_test::taskq_idx<N, int, decltype(tup)>(tup);
  EXPECT_EQ(0, idx);
}

}
