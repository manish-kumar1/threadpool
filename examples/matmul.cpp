#include <iostream>
#include <string>
#include <array>
#include <random>
#include <tuple>
#include <chrono>
#include <cassert>
#include <execution>
#include <glog/logging.h>

#include "include/threadpool.hpp"
#include "include/partitioner.hpp"
#include "include/clock_util.hpp"

namespace rng = std::ranges;

using Val = int;
constexpr size_t N = 256*4;
using Matrix = std::array<std::array<Val, N>, N>;

Matrix mat1, mat2, ans1, ans2;

decltype(auto) matmul_normal(const Matrix& mat1, const Matrix& mat2, Matrix& ans, size_t n)
{
  for(auto i = 0u; i < n; ++i)
    for(auto j = 0u; j < n; ++j) {
      ans[i][j] = 0u;
      for(auto k = 0u; k < n; ++k)
        ans[i][j] += mat1[i][k]*mat2[k][j];
    }
}

decltype(auto) matmul_tp(const Matrix& mat1, 
                         const Matrix& mat2,
                         Matrix& ans,
                         const size_t& n,
                         thp::threadpool& tp)
{
  // mxn nxm => mxm
  auto mul3 = [&](const size_t cs, const size_t ce, const size_t step) {
    auto tile = [&] (const size_t j, const size_t rs, const size_t re, std::shared_ptr<std::array<Val, N>> col) {
      for (size_t i = rs; i < re; ++i) {
        ans[i][j] = std::inner_product(mat1[i].begin(), mat1[i].begin()+n, (*col).cbegin(), 0, std::plus<>(), std::multiplies<>());
      }
    };

    for (auto c = cs; c < ce; ++c)
      for(size_t i = 0; i < n; i += step) {
        auto col_vec = std::make_shared<std::array<Val, N>>();
        rng::transform(mat2, col_vec->begin(), [c](auto&& r) { return r[c]; });
        tp.enqueue(tile, c, i, std::min(n, i+step), col_vec);
      }
  };
      
  auto mul2 = [&](size_t rs, size_t re, size_t step) {
    auto tile = [&] (const size_t i, const size_t n, const size_t cs, const size_t ce) {
                    for(size_t j = cs; j < ce; ++j) {
                       ans[i][j] = 0;
                       for (size_t k = 0; k < n; ++k)
                         ans[i][j] += mat1[i][k]*mat2[k][j];
                    }
                  };
    for(auto i = rs; i < re; ++i) {
      for(size_t c = 0; c < n; c += step)
        tp.enqueue(tile, i, n, c, std::min(n, c+step));
    }
  };

  auto mul = [&](const size_t i, const size_t cs, const size_t ce) -> void {
      // std::cerr << rs << ":" << re << ":" << cs << ":" << ce << ", n= " << n << std::endl;
              //for(auto i = rs; i < re; ++i)
                for(auto j = cs; j < ce; ++j) {
                  ans[i][j] = 0u;
                  for(auto k = 0u; k < n; ++k)
                    ans[i][j] += mat1[i][k]*mat2[k][j];
                }
            };
#if 0
  const size_t step = static_cast<int>(sqrt(n));
  for(auto i = 0u; i < n; i += step)
    for(auto j = 0u; j < n; j += step) {
      tp.schedule(thp::make_task(mul, i, std::min(n, i+step), j, std::min(n, j+step)));
    }
  for(auto i = 0; i < n; ++i)
    tp.enqueue(mul, i, 0, n);
#endif
  //mul2(0, n, 256);
  mul3(0, n, 1024u);
  tp.drain();
}

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  //google::InitGoogleLogging(argv[0]);

  bool verify = false;
  try {
    // generate data
    for(size_t n = 128; n <= N; n += 128) {
      std::random_device r;
      std::default_random_engine engine(r());
      std::uniform_int_distribution<Val> dis(-4200, +4200);

      rng::for_each(mat1, [](auto& m) { rng::fill(m, 0); });
      rng::for_each(mat2, [](auto& m) { rng::fill(m, 0); });
      rng::for_each(ans1, [](auto& m) { rng::fill(m, 0); });
      rng::for_each(ans2, [](auto& m) { rng::fill(m, 0); });

      for(auto i = 0u; i < n; ++i) {
        auto& row1 = mat1[i];
        auto& row2 = mat2[i];
        std::generate_n(row1.begin(), n, [&] { return dis(engine); });
        std::generate_n(row2.begin(), n, [&] { return dis(engine); });
      }
      {
        thp::threadpool tp;
        thp::util::clock_util<std::chrono::steady_clock> cu;
        cu.now();
        matmul_tp(mat1, mat2, ans1, n, tp);
        cu.now();
        std::cerr << "thp:    (" << n << "x" << n << ") = " << cu.get_ms() << " ms" << std::endl;
        tp.shutdown();
      }
#if 0
      {
        thp::util::clock_util<std::chrono::steady_clock> cu;
        cu.now();
        matmul_normal(mat1, mat2, ans2, n);
        cu.now();
        std::cerr << "normal: (" << n << "x" << n << ") = " << cu.get_ms() << " ms" << std::endl;
      }
#endif
      if (verify)
      {
        bool passed = true;
        for(auto i = 0u; i < n; ++i)
          if (!rng::equal(ans1[i], ans2[i])) {
            std::cerr << "failed at row " << i << std::endl;
            passed = false;
            break;
          }
        std::cerr << (passed ? "passed" : "failed") << std::endl;
      }
    }
  } catch(std::exception& ex) {
    std::cerr << "main: Exception: " << ex.what() << std::endl;
  } catch(...) {
    std::cerr << "Exception: " << std::endl;
  }
  return 0;
}

