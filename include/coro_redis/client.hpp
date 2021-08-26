//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <hiredis/hiredis.h>
#include <coro_redis/task.hpp>
#include <coro_redis/connection.hpp>

struct event_base;

namespace coro_redis {
class client final {
public:
	using awaiter_t = task_awaiter<connection, const redisAsyncContext*>;
	struct timeval_t {
		long    tv_sec;         /* seconds */
		long    tv_usec;        /* and microseconds */
	};

    // 静态单例
    static client& get() {
        static client clt;
        return clt;
    }

	awaiter_t connect(event_base* base, std::string_view host_sv, uint16_t port, long timeout_seconds = 5) {
		return awaiter_t([base, host = std::string(host_sv), port, timeout_seconds](awaiter_t* awaiter, const coro::coroutine_handle<>&) {
			ASSERT_RETURN(base, void(0), "event base is nullptr");
			ASSERT_RETURN(!host.empty(), void(0), "redis host not set");
			ASSERT_RETURN(port > 0, void(0), "redis port not set");

			LOG_INFO("connect to redis: {}:{}", host, port);

			timeval_t timeout = { timeout_seconds, 0 }; // 1.5 seconds
			redisOptions opt{};
			std::string sh(host.data(), host.size());
			opt.endpoint.tcp.ip = sh.c_str();
			opt.endpoint.tcp.port = port;
			opt.connect_timeout = (const timeval*)&timeout;
			opt.command_timeout = (const timeval*)&timeout;
			redisAsyncContext* actx = redisAsyncConnectWithOptions(&opt);
			ASSERT_RETURN(actx != nullptr, void(0), "connect redis failed, {}:{}", host, port);
			ASSERT_RETURN(actx->err == 0, void(0), "connect redis failed, {}:{}", actx->err, actx->errstr);
			ASSERT_RETURN(redisLibeventAttach(actx, base) == REDIS_OK, void(0), "redis event attach failed.");
			actx->data = awaiter;
			redisAsyncSetConnectCallback(actx, [](const struct redisAsyncContext* actx, int status) {
				auto* awaiter = (awaiter_t*)actx->data;
				if (status == REDIS_OK) {
					awaiter->set_coro_return(actx);
					LOG_INFO("redis connect success.", status);
				}
				else {
					LOG_ERROR("redis connect error, {}({})", actx->errstr, actx->err);
					awaiter->set_coro_return(nullptr);
				}
				awaiter->resume();
				});
			redisAsyncSetDisconnectCallback(actx, [](const struct redisAsyncContext* actx, int status) {
				LOG_INFO("redis disconnect status: {}", status);
				auto* awaiter = (awaiter_t*)actx->data;
				awaiter->set_coro_return(nullptr);
				});
			}, [](awaiter_t* awaiter, const coro::coroutine_handle<>&) -> std::optional<connection> {
				ASSERT_RETURN(awaiter->coro_return().has_value(), std::nullopt, "redis connect failed.");
				return std::move(connection((redisAsyncContext*)(awaiter->coro_return().value())));
			});
    }

private:
    client() = default;
    client(const client&) = delete;
    void operator =(const client&) = delete;
}; // class client
} // namespace coro_redis