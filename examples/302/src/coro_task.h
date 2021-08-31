#pragma once

#include <any>
#include <optional>
#include <coroutine>

#include <logger.hpp>

#ifdef WIN32
namespace coro = std;
#else
namespace coro = std;
#endif // WIN32



template<typename TASK_RET> class task_promise;

struct nothing { };

template <typename T>
struct void_to_nothing {
    using type = T;
};

template <>
struct void_to_nothing<void> {
    using type = nothing;
};


template<typename TASK_RET>
class task {
  public:
    using promise_type = task_promise<TASK_RET>;
    using value_type = TASK_RET;
    friend class task_promise<TASK_RET>;

  public:
    task(coro::coroutine_handle<> h) {
        h_coro_ = h;
    }

    ~task() {  }

    auto& handler() {
        return h_coro_;
    }

    const auto& return_value() {
        return ret_value_;
    }

  private:
    void_to_nothing<TASK_RET>::type ret_value_{};
    coro::coroutine_handle<> h_coro_;
};

class task_promise_base {
  public:
    auto initial_suspend() {
        return coro::suspend_never{};
    }

    auto final_suspend() noexcept {
        return coro::suspend_never{};
    }

    void unhandled_exception() {
        m_exception = std::current_exception();
    }

  private:
    std::exception_ptr m_exception;
};

struct get_promise_t {};
constexpr get_promise_t get_promise = {};

template<typename TASK_RET>
class task_promise final : public task_promise_base {
  public:
    using task_type = task<TASK_RET>;
    task<TASK_RET> get_return_object() noexcept {
        return task<TASK_RET>(coro::coroutine_handle<task_promise>::from_promise(*this));
    }

    ~task_promise() {
        LOG_DEBUG("~task_promise");
    }

    void return_value(TASK_RET value) {
        LOG_DEBUG("return_value: {}", value);

        auto h = coro::coroutine_handle<task_promise>::from_promise(*this);
        auto task = reinterpret_cast<task_type*>(h.address());
        task->ret_value_ = value;
    }
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

template<typename CORO_RET>
struct task_awaiter {
    typedef std::function<void(task_awaiter<CORO_RET>* awaiter, const coro::coroutine_handle<>&)> suspend_cb_type;
    typedef std::function<std::optional<CORO_RET>(task_awaiter<CORO_RET>* awaiter, const coro::coroutine_handle<>&)> resume_cb_type;
    task_awaiter() = default;
    task_awaiter(suspend_cb_type suspend_callback, resume_cb_type resume_callback) :
        suspend_callback_(suspend_callback),
        resume_callback_(resume_callback),
        want_pause_(true) {
    }

    bool await_ready() const noexcept {
        return !want_pause_;
    }

    void await_suspend(coro::coroutine_handle<> coroutine) {
        h_coro_ = coroutine;
        if (suspend_callback_) suspend_callback_(this, h_coro_);
    }

    std::optional<CORO_RET> await_resume() noexcept {
        if (resume_callback_) return resume_callback_(this, h_coro_);
        return std::nullopt;
    }

    void set_coro_return(std::any coro_ret) {
        if (coro_ret.has_value()) coro_tmp_return_ = coro_ret;
    }

    std::any& coro_return() {
        return coro_tmp_return_;
    }

    void resume() {
        h_coro_.resume();
    }

  private:
    suspend_cb_type suspend_callback_;
    resume_cb_type resume_callback_;
    bool want_pause_ = false;
    coro::coroutine_handle<> h_coro_;

    std::any coro_tmp_return_{};
};
