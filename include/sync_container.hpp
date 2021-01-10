#ifndef FUTURE_RANGES_HPP__
#define FUTURE_RANGES_HPP__

#include <future>
#include <ranges>
#include <iterator>
#include <mutex>
#include <vector>

namespace thp {
namespace details {

template <typename Container, typename Inner_iterator>
class sync_container_iterator;

} // namespace details


// sync container adapter
template<typename T, typename Container = std::vector<T>>
class sync_container {
private:
  using inner_iterator = typename Container::iterator;
  using const_inner_iterator = typename Container::const_iterator;

  template<typename C, typename I>
  friend class details::sync_container_iterator;

public:

  using this_type = sync_container<T, Container>;
  using iterator = details::sync_container_iterator<this_type, inner_iterator>;
  using const_iterator = details::sync_container_iterator<this_type, const_inner_iterator>;

  //explicit sync_container(std::shared_ptr<std::vector<std::future<Ret>>> ret)
  constexpr explicit sync_container()
  : mu{}
  , cond{}
  , valid_size{0}
  , data{}
  {}

  sync_container(const sync_container&) = default;
  sync_container& operator = (const sync_container& ) = default;

  sync_container(sync_container&&) = default;
  sync_container& operator = (sync_container&&) = default;

  decltype(auto) size() {
    std::unique_lock l(mu);
    return data.size();
  }

  decltype(auto) put(T& item) {
    std::unique_lock l(mu);
    data.emplace_back(item);
    ++valid_size;
    cond.notify_one();
    return *this;
  }

  decltype(auto) put(const T& item) {
    std::unique_lock l(mu);
    data.emplace_back(item);
    ++valid_size;
    cond.notify_one();
    return *this;
  }

  decltype(auto) put(T&& item) {
    std::unique_lock l(mu);
    data.emplace_back(std::move(item));
    ++valid_size;
    cond.notify_one();
    return *this;
  }

  iterator begin()
  {
    return iterator(this, data.begin());
  }

  const_iterator cbegin() const
  {
    return const_iterator(this, data.cbegin());
  }

  iterator end()
  {
    return iterator(this, data.end());
  }

  const_iterator cend() const
  {
    return const_iterator(this, data.cend());
  }

  T& operator[](size_t idx) {
    //return accepts(std::next(data.begin(), idx));
    return accept(idx);
  }

  const T& operator [](size_t idx) const {
    return accept(idx);
//    return accepts(std::next(data.cbegin(), idx));
  }

  const T& at(size_t idx) const {
    return accepts(std::next(data.cbegin(), idx));
  }

  T& at(size_t idx) {
    return accepts(std::next(data.begin(), idx));
  }

private:
  const T& accept(size_t at) const {
    std::cerr << "accept(" << at << ")" << std::endl;
    std::unique_lock l(mu);
    cond.wait(l, [&] { return at < valid_size; });
    return data.at(at);
  }

  T& accept(size_t at) {
    std::cerr << "accept(" << at << ")" << std::endl;
    std::unique_lock l(mu);
    cond.wait(l, [&] { return at < valid_size; });
    return data.at(at);
  }
  
  const T& accepts(const_inner_iterator it) const {
    std::unique_lock l(mu);
    return *it;
  }

  T& accepts(inner_iterator it) {
    std::unique_lock l(mu);
    return *it;
  }

private:
  std::mutex mu;
  std::condition_variable_any cond;
  size_t valid_size; 
  Container data;
};

namespace details {

template <typename SyncContainer, typename InnerIterator>
class sync_container_iterator {
private:
  SyncContainer* container;
  InnerIterator iter;
  size_t idx;

public:
  explicit sync_container_iterator(SyncContainer* c, InnerIterator it)
  : container{c}
  , iter{it}
  , idx{std::distance(c->data.begin(), iter)}
  { }

  decltype(auto) operator * () { return (*container)[idx]; }
  sync_container_iterator& operator ++() { ++iter; ++idx; return *this; }
  sync_container_iterator& operator ++(int) { iter++; ++idx; return *this; }

  friend bool operator == (sync_container_iterator a, sync_container_iterator b) {
    return (a.container == b.container) && (a.iter == b.iter);
  }

  friend bool operator != (sync_container_iterator a, sync_container_iterator b) {
    return !(a == b);
  }
};

} //namespace details
} // namespace thp
#endif // FUTURE_RANGES_HPP__
