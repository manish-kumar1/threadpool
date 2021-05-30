#ifndef __WORKER_FN__
#define __WORKER_FN__

#include <variant>
#include <functional>
#include <optional>
#include <memory>

namespace thp {

template<typename T>
class thread_fn {
public:
  constexpr explicit thread_fn(ThpWorkerVariant&& fn) : var{std::move(fn)} {}

  thread_fn(const thread_fn&) = delete;
  thread_fn& operator=(const thread_fn) = delete;

  thread_fn(thread_fn&&) = default;
  ~thread_fn() = default;

  constexpr void operator()() {
    auto config = std::get<T>(var);
    for(;;) {
      try {
        auto init_lambda = config->initializer();
        auto work_loop   = config->work_loop();
        auto finalizer   = config->finalizer();

        if (init_lambda) std::invoke(init_lambda.value());
        if (work_loop) std::invoke(work_loop.value(), config->stop_token());
      }
      catch(worker_config_change_except &ex) {}
      catch(worker_stop_except& ex) {
        if (finalizer) std::invoke(finalizer.value());
        break;
      }
      catch(...) {
        throw;
      }
    }
  }

private:
  ThpWorkerVariant var;
};

} // namespace thp

#endif // __WORKER_FN__
