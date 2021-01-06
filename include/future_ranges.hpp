#ifndef FUTURE_RANGES_HPP__
#define FUTURE_RANGES_HPP__

#include <future>
#include <ranges>
#include <iterator>
#include <condition_variable>
#include <mutex>

namespace thp {
namespace details {

template <typename Container>
class sync_container_iterator {
private:
  Container* container;
  typename Container::inner_iterator iter;
  size_t idx;
public:
  explicit sync_container_iterator(Container* c, typename Container::inner_iterator it)
  : container{c}
  , iter{it}
  , idx{std::distance(c->begin(), iter)}
  { }

  decltype(auto) operator * () { return container->at(idx); }
  sync_container_iterator& operator ++() { ++idx; return *this; }
  sync_container_iterator& operator ++(int) { ++idx; return *this; }

  friend bool operator == (sync_container_iterator a, sync_container_iterator b) {
    return (a.c == b.c) && (a.at == b.at);
  }

  friend bool operator != (sync_container_iterator a, sync_container_iterator b) {
    return !(a == b);
  }
};

} //namespace details

template<typename T, typename Container = std::vector<T>>
class sync_container {
  using inner_iterator = typename Container::iterator;
  using const_inner_iterator = typename Container::const_iterator;

public:
  friend class sync_container_iterator<Container>;

  //explicit sync_container(std::shared_ptr<std::vector<std::future<Ret>>> ret)
  explicit sync_container()
  : mu{}
  , cond{}
  , data{}
  {}

  decltype(auto) put(T&& item) {
    std::unique_lock l(mu);
    data.emplace_back(std::forward<T>(item));
    cond.notify_one();
    return *this;
  }

  decltype(auto) begin() {
    return details::sync_container_iterator<decltype(this)>(this, data.begin());
  }

  decltype(auto) end() {
    return details::sync_container_iterator<decltype(this)>(this, data.end());
  }

  T&& operator [](size_t idx) { return at(idx); }

  T&& at(size_t idx) {
    std::unique_lock l(mu);
    cond.wait(l, [&] { return idx < data.size(); });
    return data->at(idx);
  }
  
private:
  std::mutex mu;
  std::condition_variable_any cond;
  Container data;
};

} // namespace thp
#endif // FUTURE_RANGES_HPP__
