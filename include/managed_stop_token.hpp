#ifndef MANAGED_STOP_TOKEN__
#define MANAGED_STOP_TOKEN__

#include <stop_token>
#include <memory>

#include "include/managed_stop_source.hpp"

namespace thp {

struct managed_stop_token : std::stop_token {
  explicit managed_stop_token(std::shared_ptr<managed_stop_source> src)
  : std::stop_token{src->get_token()}
  , source{src}
  {
    //std::cerr << "managed_stop_token(): " << source.get() << "," << src.get() << ", " << source.use_count() << std::endl;
  }

  managed_stop_token(managed_stop_token&& rhs) : std::stop_token(rhs) {
    source = std::move(rhs.source);
    rhs.source = nullptr;
  }

  friend std::ostream& operator << (std::ostream& oss, const managed_stop_token& tok) {
    oss << "managed_stop_token: " << tok.source.get() << "::" << tok.source.use_count();
    return oss;
  }

  managed_stop_token& operator = (managed_stop_token&& rhs) {
    std::stop_token::operator=(std::move(rhs));
    return *this;
  }

  bool pause_requested() const { return source->pause_requested(); }

  void pause() {
    source->pause();
  }

  virtual ~managed_stop_token() = default;

protected:
  std::shared_ptr<managed_stop_source> source;
};

} // namespace thp

#endif // MANAGED_STOP_TOKEN__
