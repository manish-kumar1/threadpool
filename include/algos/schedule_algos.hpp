#ifndef SCHEDULE_ALGOS_HPP_
#define SCHEDULE_ALGOS_HPP_

#include <thread>
#include <limits>

#include "include/statistics.hpp"

namespace thp {
namespace sched_algos {

enum names : uint8_t
{
  eFirstAvailable = 0,
  eMaxLen = 1,
  eFairShare = 2,

  eInvalid = std::numeric_limits<uint8_t>::max()
};

// algo interface
struct schedule_algo {
  virtual bool ok(const statistics&) { return false; }
  virtual void apply(const statistics& ) = 0;
  virtual int apply(const statistics&, std::thread::id ) = 0;
};

} // namespace sched_algos
} // namespace thp

#endif // SCHEDULE_ALGOS_HPP_
