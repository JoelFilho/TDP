#ifndef TDP_BLOCKING_QUEUE_HPP
#define TDP_BLOCKING_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace tdp::util {

template <typename T>
class blocking_queue {
 public:
  void push(T val) {
    {
      std::unique_lock lock{_mutex};
      _queue.push(std::move(val));
    }
    _condition.notify_one();
  }

  T pop() {
    std::unique_lock lock{_mutex};
    _condition.wait(lock, [&] { return !_queue.empty(); });
    auto r = std::move(_queue.front());
    _queue.pop();
    return r;
  }

  template <typename Pred>
  std::optional<T> pop_unless(Pred&& p) {
    std::unique_lock lock{_mutex};
    _condition.wait(lock, [&] { return p() || !_queue.empty(); });

    if (_queue.empty())
      return std::nullopt;

    auto r = std::move(_queue.front());
    _queue.pop();
    return {r};
  }

  bool empty() const noexcept { return _queue.empty(); }

  void wake() {
    std::unique_lock lock{_mutex};
    _condition.notify_all();
  }

 private:
  std::queue<T> _queue;
  std::mutex _mutex;
  std::condition_variable _condition;
};

}  // namespace tdp::util

#endif