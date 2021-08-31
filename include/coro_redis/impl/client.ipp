//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once
#include <list>
#include <mutex>
#include <hiredis/async.h>

#include <coro_redis/context.hpp>
#include <coro_redis/coro_connection.hpp>
#include <coro_redis/sync_connection.hpp>

namespace coro_redis {
namespace impl {
struct timeval_t {
  long tv_sec;  /* seconds */
  long tv_usec; /* and microseconds */
};

class client_impl {
 private:
  using awaiter_t = task_awaiter<std::shared_ptr<coro_connection>, const redisAsyncContext*>;
  using fetch_awaiter_t = task_awaiter<std::shared_ptr<coro_connection>>;
 public:
  awaiter_t coro_connect(const io_context& ioc, std::string_view host_sv,
                         uint16_t port, long timeout_seconds) {
    return awaiter_t(
        [&ioc, host = std::string(host_sv), port, timeout_seconds](
            awaiter_t* awaiter, const coro::coroutine_handle<>&) {
          ASSERT_RETURN(!host.empty(), void(0), "redis host not set");
          ASSERT_RETURN(port > 0, void(0), "redis port not set");

          LOG_INFO("coro connect to redis: {}:{}", host, port);

          timeval_t timeout = {timeout_seconds, 0};  // 1.5 seconds
          redisOptions opt{};
          std::string sh(host.data(), host.size());
          opt.endpoint.tcp.ip = sh.c_str();
          opt.endpoint.tcp.port = port;
          opt.connect_timeout = (const timeval*)&timeout;
          opt.command_timeout = (const timeval*)&timeout;
          redisAsyncContext* actx = redisAsyncConnectWithOptions(&opt);
          ASSERT_RETURN(actx != nullptr, void(0), "connect redis failed, {}:{}",
                        host, port);
          ASSERT_RETURN(actx->err == 0, void(0), "connect redis failed, {}:{}",
                        actx->err, actx->errstr);
          ASSERT_RETURN(ioc.attach(actx) == REDIS_OK, void(0),
                        "redis event attach failed.");
          actx->data = awaiter;
          redisAsyncSetConnectCallback(
              actx, [](const struct redisAsyncContext* actx, int status) {
                auto* awaiter = (awaiter_t*)actx->data;
                if (status == REDIS_OK) {
                  awaiter->set_coro_return(actx);
                  LOG_INFO("redis connect success.", status);
                } else {
                  LOG_ERROR("redis connect error, {}({})", actx->errstr,
                            actx->err);
                  awaiter->set_coro_return(nullptr);
                }
                awaiter->resume();
              });
          redisAsyncSetDisconnectCallback(
              actx, [](const struct redisAsyncContext* actx, int status) {
                LOG_INFO("redis disconnect status: {}", status);
                auto* awaiter = (awaiter_t*)actx->data;
                awaiter->set_coro_return(nullptr);
              });
        },
        [](awaiter_t* awaiter,
           const coro::coroutine_handle<>&) -> std::optional<std::shared_ptr<coro_connection>> {
          ASSERT_RETURN(awaiter->coro_return().has_value(), std::nullopt,
                        "redis connect failed.");
          std::shared_ptr<coro_connection> conn =
              std::make_shared<coro_connection>(
                  (redisAsyncContext*)awaiter->coro_return().value());
          return conn;
        });
  }

  std::shared_ptr<sync_connection> sync_connect(std::string_view host,
                                                uint16_t port,
                                                long timeout_seconds) {
    ASSERT_RETURN(!host.empty(), nullptr, "redis host not set");
    ASSERT_RETURN(port > 0, nullptr, "redis port not set");

    LOG_INFO("sync connect to redis: {}:{}", host, port);

    timeval_t timeout = {timeout_seconds, 0};  // 1.5 seconds
    redisOptions opt{};
    std::string sh(host.data(), host.size());
    opt.endpoint.tcp.ip = sh.c_str();
    opt.endpoint.tcp.port = port;
    opt.connect_timeout = (const timeval*)&timeout;
    opt.command_timeout = (const timeval*)&timeout;

    auto* ctx = redisConnectWithOptions(&opt);
    ASSERT_RETURN(ctx != nullptr, nullptr, "redis connect failed.");
    return std::make_shared<sync_connection>(ctx);
  }

	void pool_init(std::vector<io_context*> pool_ios,
                 std::string_view host_sv,
                 uint16_t port,
                 long timeout_seconds = 5) {
    std::lock_guard<std::mutex> locker(pool_mutex_);
    pool_ios_ = pool_ios;
    host_ = host_sv;
    port_ = port;
    timeout_ = timeout_seconds;
  }

  fetch_awaiter_t fetch_coro_conn() {
    return fetch_awaiter_t(
        [this](fetch_awaiter_t* awaiter, const coro::coroutine_handle<>&) {
          std::lock_guard<std::mutex> locker(pool_mutex_);
          if (!this->free_pool_.empty()) {
            // find available connection
            awaiter->resume();
            return;
          }
          // no available connection, try to create one
          if (!this->pool_ios_.empty())
          {
            auto* ioc = pool_ios_.back();
            pool_ios_.pop_back();
            timeval_t timeout = {timeout_, 0};  // 1.5 seconds
            redisOptions opt{};
            std::string sh(host_.data(), host_.size());
            opt.endpoint.tcp.ip = sh.c_str();
            opt.endpoint.tcp.port = port_;
            opt.connect_timeout = (const timeval*)&timeout;
            opt.command_timeout = (const timeval*)&timeout;
            redisAsyncContext* actx = redisAsyncConnectWithOptions(&opt);
            ASSERT_RETURN(actx != nullptr, void(0),
                          "connect redis failed, {}:{}", host_, port_);
            ASSERT_RETURN(actx->err == 0, void(0),
                          "connect redis failed, {}:{}", actx->err,
                          actx->errstr);
            ASSERT_RETURN(ioc->attach(actx) == REDIS_OK, void(0),
                          "redis event attach failed.");
            awaiter->set_coro_return(this);
            actx->data = awaiter;
            redisAsyncSetConnectCallback(
                actx, [](const struct redisAsyncContext* actx, int status) {
                  auto* awaiter = (fetch_awaiter_t*)actx->data;
                  client_impl* clt = std::any_cast<client_impl*>(
                      awaiter->coro_return().value());
                  clt->add_new_conn((redisAsyncContext*)actx);
                  awaiter->resume();
                });
            redisAsyncSetDisconnectCallback(
                actx, [](const struct redisAsyncContext* actx, int status) {
                  LOG_INFO("redis disconnect status: {}", status);
                  auto* awaiter = (fetch_awaiter_t*)actx->data;
                  awaiter->resume();
                });
            return;
          }
          // if no available context, create failed, wait other connection to free
          {
            fetch_awaiters_.push_back(awaiter);
            return;
          }
        },
        [](fetch_awaiter_t* awaiter, const coro::coroutine_handle<>&)
            -> std::optional<std::shared_ptr<coro_connection>> {
          client_impl* clt =
              std::any_cast<client_impl*>(awaiter->coro_return().value());
          return clt->fetch_from_free_pool();
        });
  }

  std::shared_ptr<coro_connection> sync_fetch_conn() {


    auto conn = fetch_from_free_pool();
    if (conn) return conn;

    std::lock_guard<std::mutex> locker(pool_mutex_);
    // no available connection, try to create one
    if (!pool_ios_.empty()) {
      auto* ioc = pool_ios_.back();
      pool_ios_.pop_back();
      timeval_t timeout = {timeout_, 0};  // 1.5 seconds
      redisOptions opt{};
      std::string sh(host_.data(), host_.size());
      opt.endpoint.tcp.ip = sh.c_str();
      opt.endpoint.tcp.port = port_;
      opt.connect_timeout = (const timeval*)&timeout;
      opt.command_timeout = (const timeval*)&timeout;
      redisAsyncContext* actx = redisAsyncConnectWithOptions(&opt);
      ASSERT_RETURN(actx != nullptr, nullptr, "connect redis failed, {}:{}",
                    host_, port_);
      ASSERT_RETURN(actx->err == 0, nullptr, "connect redis failed, {}:{}",
                    actx->err, actx->errstr);
      ASSERT_RETURN(ioc->attach(actx) == REDIS_OK, nullptr,
                    "redis event attach failed.");
      return add_new_conn((redisAsyncContext*)actx);
    }
    // if no available context, create failed,
    // we cannot wait other connection to free, so return nullptr
    return nullptr;
  }

  std::shared_ptr<coro_connection> fetch_from_free_pool() {
    std::lock_guard<std::mutex> locker(pool_mutex_);
    if (free_pool_.empty()) return nullptr;
    auto conn = free_pool_.front();
    free_pool_.pop_front();
    inuse_pool_.push_back(conn);
    return conn;
  }

  std::shared_ptr<coro_connection> add_new_conn(redisAsyncContext* actx) {
    std::lock_guard<std::mutex> locker(pool_mutex_);
    std::shared_ptr<coro_connection> conn(
        new coro_connection(actx), [this](auto p) { recycle_conn(p); });
    inuse_pool_.push_back(conn);
    return conn;
  }

  void recycle_conn(coro_connection* p) {
    std::lock_guard<std::mutex> locker(pool_mutex_);
    auto iter = std::find_if(inuse_pool_.begin(), inuse_pool_.end(),
                             [p](auto pp) { return pp.get() == p; });
    if (iter == inuse_pool_.end()) return;
    free_pool_.push_back(*iter);
  }

 private:
  std::mutex pool_mutex_;
  std::string host_;
  uint16_t port_ = 0;
  long timeout_ = 0;
  std::vector<io_context*> pool_ios_;

  std::list<std::shared_ptr<coro_connection>> free_pool_;
  std::list<std::shared_ptr<coro_connection>> inuse_pool_;

  std::list<fetch_awaiter_t*> fetch_awaiters_;
};

// bool create_conn_pool(const std::vector<event_base*> contexts,
// std::string_view host_sv, uint16_t port, long timeout_seconds) { 	return true;
//}

}  // namespace impl
}  // namespace coro_redis