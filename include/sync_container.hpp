#ifndef FUTURE_RANGES_HPP__
#define FUTURE_RANGES_HPP__

#include <iostream>

#include <future>
#include <ranges>
#include <iterator>
#include <shared_mutex>
#include <vector>

namespace thp {
namespace details {

template <typename Container, typename Inner_iterator>
class sync_container_iterator;

template<typename Iter>
struct sync_container_sentinel {
  const std::iterator_traits<Iter>::difference_type idx;
};

} // namespace details

//
// sync buffer container adapter
// store elements of type T, allow concurrent read/write/access
// 
template<typename T, typename Container = std::vector<T>>
class sync_container {
private:
  using inner_iterator = typename Container::iterator;
  using const_inner_iterator = typename Container::const_iterator;

  template<typename C, typename I>
  friend class details::sync_container_iterator;

  std::shared_mutex mu;
  std::condition_variable_any cond;
  size_t cur_size;
  size_t buf_len;
  Container data;

public:

  using this_type = sync_container<T, Container>;
  using iterator = details::sync_container_iterator<this_type, inner_iterator>;
  using const_iterator = details::sync_container_iterator<this_type, const_inner_iterator>;

  explicit sync_container(size_t buf_size)
  : mu{}
  , cond{}
  , cur_size{0}
  , buf_len{buf_size}
  , data{}
  {
    data.reserve(buf_len);
  }

  auto size() {
    std::shared_lock l(mu);
    return data.size();
  }

  auto reserve(size_t n)
  {
    std::unique_lock l(mu);
    data.reserve(n);
    buf_len = n;
    cur_size = 0;
  }

  decltype(auto) put(T& item) {
    std::lock_guard l(mu);
    data.emplace_back(item);
    ++cur_size;
    cond.notify_one();
    return *this;
  }

  decltype(auto) put(const T& item) {
    std::lock_guard l(mu);
    data.emplace_back(item);
    ++cur_size;
    cond.notify_one();
    return *this;
  }

  decltype(auto) put(T&& item) {
    std::lock_guard l(mu);
    data.emplace_back(std::move(item));
    ++cur_size;
    cond.notify_one();
    return *this;
  }

  constexpr iterator begin()              { return iterator(this, data.begin()); }
  constexpr const_iterator cbegin() const { return const_iterator(this, data.cbegin()); }
  constexpr auto end()                    { return details::sync_container_sentinel<inner_iterator>(buf_len); }
  constexpr auto cend() const             { return end(); }

  T& operator[](size_t idx) {
    if (idx > buf_len) {
      std::ostringstream err;
      err << idx << " > " << buf_len;
      throw std::out_of_range(err.str());
    }
    return accept(idx);
  }

  const T& operator [](size_t idx) const {
    if (idx > buf_len) {
      std::ostringstream err;
      err << idx << " > " << buf_len;
      throw std::out_of_range(err.str());
    }
    return accept(idx);
  }

private:
  const T& accept(size_t at) const {
    std::unique_lock l(mu);
    //std::cerr << "const accept(" << at << ")" << at << ", " << cur_size << ", " << buf_len << std::endl;
    cond.wait(l, [&] { return at < cur_size; });
    return *std::next(data.cbegin(), at);
  }

  T& accept(size_t at) {
    std::unique_lock l(mu);
    //std::cerr << "accept(" << at << ")" << at << ", " << cur_size << ", " << buf_len << std::endl;
    cond.wait(l, [&] { return at < cur_size; });
    return *std::next(data.begin(), at);
  }
};

namespace details {

template <typename SyncContainer, typename InnerIterator>
class sync_container_iterator {
private:
  SyncContainer* container;
  typename std::iterator_traits<InnerIterator>::difference_type idx;

public:
  explicit sync_container_iterator(SyncContainer* c, InnerIterator it)
  : container{c}
  , idx{std::distance(c->data.begin(), it)}
  { }

  decltype(auto) operator * () { return (*container)[idx]; }
  sync_container_iterator& operator ++() { ++idx; return *this; }
  sync_container_iterator& operator ++(int) { ++idx; return *this; }

  friend bool operator == (sync_container_iterator a, sync_container_iterator b) {
    return (a.container == b.container) && (a.idx == b.idx);
  }

  friend bool operator == (sync_container_iterator a, sync_container_sentinel<InnerIterator> b) {
    return a.idx >= b.idx;
  }
};

} // namespace details
} // namespace thp

#endif // FUTURE_RANGES_HPP__
