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

using Val = int;
constexpr size_t N = 1024+512;
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
  //std::vector<thp::simple_task<void>> tasks;
  auto mul = [&](size_t rs, size_t re, size_t cs, size_t ce) -> void {
      //        std::cerr << rs << ":" << re << ":" << cs << ":" << ce << ", n= " << n << std::endl;
              for(auto i = rs; i < re; ++i)
                for(auto j = cs; j < ce; ++j) {
                  ans[i][j] = 0u;
                  for(auto k = 0u; k < n; ++k)
                    ans[i][j] += mat1[i][k]*mat2[k][j];
                }
            };

  const size_t step = 64;
  for(auto i = 0u; i < n; i += step)
    for(auto j = 0u; j < n; j += step) {
      tp.schedule(thp::make_task(mul, i, std::min(n, i+step), j, std::min(n, j+step)));
    }

  tp.drain();
}

int main(int argc, const char* const argv[])
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  bool compare = false;

  try {
    // generate data
    for(size_t n = 0; n < N; n += 256) {
      std::random_device r;
      std::default_random_engine engine(r());
      std::uniform_int_distribution<Val> dis(-4200, +4200);
      for(auto i = 0u; i < n; ++i) {
        auto& row1 = mat1[i];
        auto& row2 = mat2[i];
        std::generate_n(row1.begin(), n, [&] { return dis(engine); });
        std::generate_n(row2.begin(), n, [&] { return dis(engine); });
        std::fill_n(ans1[i].begin(), n, 0);
        std::fill_n(ans2[i].begin(), n, 0);
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
      if (compare)
      {
        bool passed = true;
        for(auto i = 0u; i < n; ++i)
          if (!std::equal(ans1[i].begin(), ans1[i].begin()+n, ans2[i].begin())) {
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

