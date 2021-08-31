#pragma once

namespace coro_redis {
struct io_context {
    virtual int attach(redisAsyncContext* actx) const = 0;
    virtual void loop() const = 0;
    virtual void exit() const = 0;
};

#ifdef __HIREDIS_LIBEVENT_H__
class libevent_io_context : public io_context {
  public:
    libevent_io_context() : base_(event_base_new()) {}
    ~libevent_io_context() {
        if (base_) event_base_free(base_);
    }

    virtual int attach(redisAsyncContext* actx) const override {
        return redisLibeventAttach(actx, base_);
    }

    virtual void loop() const override {
        event_base_dispatch(base_);
    }

    virtual void exit() const override {
        event_base_loopexit(base_, nullptr);
    }

  private:
    event_base* base_ = nullptr;
};
#endif

#ifdef __HIREDIS_LIBUV_H__
class libuv_io_context : public io_context {
  public:
  libuv_io_context() : loop_((uv_loop_t*)malloc(sizeof(uv_loop_t))) {
     uv_loop_init(loop_);
  }
    ~libuv_io_context() {
        if (loop_) uv_loop_close(loop_);
    }

    virtual int attach(redisAsyncContext* actx) const override {
        return redisLibuvAttach(actx, loop_);
    }

    virtual void loop() const override { uv_run(loop_, UV_RUN_DEFAULT); }

    virtual void exit() const override { uv_stop(loop_); }
  private:
    uv_loop_t* loop_ = nullptr;
};
#endif
}
