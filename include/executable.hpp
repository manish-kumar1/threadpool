#ifndef EXECUTABLE_HPP_
#define EXECUTABLE_HPP_

namespace thp {

// task interface
struct executable {
  virtual void execute() { throw std::logic_error("shouldn't be called"); };
  virtual std::ostream& info(std::ostream& oss) {
    oss << typeid(*this).name();
    return oss;
  }
  virtual ~executable() = default;
};

} // namespace thp

#endif // EXECUTABLE_HPP_
