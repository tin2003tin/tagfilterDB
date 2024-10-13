#pragma once

#include "tagfilterdb/common.hpp"

#include <mutex>
#include <shared_mutex>
#include <utility>

namespace tagfilterdb::internal {
class Mutex final {
  public:
    Mutex() = default;
    Mutex(const Mutex &) = delete;
    Mutex(Mutex &&) = delete;
    Mutex &operator=(const Mutex &) = delete;
    Mutex &operator=(Mutex &&) = delete;
    void lock() { m_impl.lock(); }
    bool try_lock() { return m_impl.try_lock(); }
    void unlock() { m_impl.unlock(); }

  private:
    std::mutex m_impl;
};

template <typename Mutex> using UniqueLock = std::unique_lock<Mutex>;

class SharedMutex final {
  public:
    SharedMutex() = default;
    SharedMutex(const SharedMutex &) = delete;
    SharedMutex(SharedMutex &&) = delete;
    SharedMutex &operator=(const SharedMutex &) = delete;
    SharedMutex &operator=(SharedMutex &&) = delete;
    void lock() { m_impl.lock(); }
    bool try_lock() { return m_impl.try_lock(); }
    void unlock() { m_impl.unlock(); }
    void lock_shared() { m_impl.lock_shared(); }
    bool try_lock_shared() { return m_impl.try_lock_shared(); }
    void unlock_shared() { m_impl.unlock_shared(); }

  private:
    std::shared_mutex m_impl;
};
template <typename Mutex> using SharedLock = std::shared_lock<Mutex>;
class OnceFlag;
template <typename Callable, typename... Args>
void call_once(OnceFlag &onceFlag, Callable &&callable, Args &&...args);

class OnceFlag final {
  public:
    constexpr OnceFlag() = default;
    OnceFlag(const OnceFlag &) = delete;
    OnceFlag(OnceFlag &&) = delete;
    OnceFlag &operator=(const OnceFlag &) = delete;
    OnceFlag &operator=(OnceFlag &&) = delete;

  private:
    template <typename Callable, typename... Args>
    friend void call_once(OnceFlag &onceFlag, Callable &&callable,
                          Args &&...args);
    std::once_flag m_impl;
};
template <typename Callable, typename... Args>
void call_once(OnceFlag &onceFlag, Callable &&callable, Args &&...args) {
    std::call_once(onceFlag.m_impl, std::forward<Callable>(callable),
                   std::forward<Args>(args)...);
}

} // namespace tagfilterdb::internal