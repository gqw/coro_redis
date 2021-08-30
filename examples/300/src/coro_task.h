#pragma once

#include <optional>
#include <coroutine>

#ifdef WIN32
namespace coro = std;
#else
namespace coro = std;
#endif // WIN32

template<typename T> class task_promise;

struct nothing { };

template <typename T>
struct void_to_nothing {
    using type = T;
};

template <>
struct void_to_nothing<void> {
    using type = nothing;
};


template<typename T = void>
class task {
  public:
    using promise_type = task_promise<T>;
    using return_type = void_to_nothing<T>::type;
    friend class promise_type;
  public:
    task(coro::coroutine_handle<promise_type> h) {
        h_coro_ = h;
    }

    ~task() { }

    auto& handler() {
        return h_coro_;
    }

    T return_value() {
        return *ret_value_;
    }

  private:
    std::shared_ptr<return_type> ret_value_;
    coro::coroutine_handle<promise_type> h_coro_;
};

class task_promise_base {
  public:
    ~task_promise_base() {}
    auto initial_suspend() {
        return coro::suspend_never{};
    }

    auto final_suspend() noexcept {
        return coro::suspend_always{};
    }

    void unhandled_exception() {
        m_exception = std::current_exception();
    }

  private:
    std::exception_ptr m_exception;
};

struct get_promise_t {};
constexpr get_promise_t get_promise = {};

template<typename T>
class task_promise final : public task_promise_base {
  public:
    using task_return_type = task<T>::return_type;
    task<T> get_return_object() noexcept {
        task<T> task(coro::coroutine_handle<task_promise>::from_promise(*this));
        ret_ = task.ret_value_ = std::make_shared<task_return_type>();
        return task;
    }

    ~task_promise() {
        LOG_DEBUG("~task_promise");
    }

    void return_value(T value) {
        LOG_DEBUG("return_value: {}", value);
        *ret_ = value;
    }
    std::shared_ptr<typename task<T>::return_type> ret_;
};


template<>
class task_promise<void> final : public task_promise_base {
  public:
    task<void> get_return_object() noexcept {
        return task<void>(coro::coroutine_handle<task_promise>::from_promise(*this));
    }

    void return_void() {
        LOG_DEBUG("return void");
    }
};

template<typename T>
struct task_awaiter {
    task_awaiter() {
        LOG_DEBUG("task_awaiter");
    }
    ~task_awaiter() {
        LOG_DEBUG("~task_awaiter");
    }
    task_awaiter(std::function<void(coro::coroutine_handle<task_promise<T>>)> suspend_callback,
                 std::function<std::optional<T>()> resume_callback) :
        suspend_callback_(suspend_callback),
        resume_callback_(resume_callback) {

    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coro::coroutine_handle<task_promise<T>> coroutine) {
        if (suspend_callback_) suspend_callback_(coroutine);
    }

    decltype(auto) await_resume() noexcept {
        return resume_callback_();
    }

  private:
    std::function<void(coro::coroutine_handle<task_promise<T>>)> suspend_callback_;
    std::function<std::optional<T>()> resume_callback_;
};
