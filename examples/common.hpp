#ifndef COMMON_EXAMPLE_HPP
#define COMMON_EXAMPLE_HPP

namespace thp {
namespace examples {

auto print_future = [](auto &&fut) {
  try {
    auto v = fut.get();
    std::cerr << "fut::value= " << v << std::endl;
  } catch (std::exception& e) {
    std::cerr << "fut::exception= " << e.what() << std::endl;
  }
};

// a highly inefficient prime checker 
bool is_prime(unsigned int n) {
  if (n <= 1) return false;
  else if (n == 2) {
    return true;
  }
  else if (n % 2 == 0)
    return false;

  for (unsigned i = 3; i < (1+sqrt(n)); i += 2) {
    if (n % i == 0) {
      return false;
    }
  }
  return true;
}


}
}

#endif // COMMON_EXAMPLE_HPP