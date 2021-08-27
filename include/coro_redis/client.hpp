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
#include <coro_redis/client.ipp>

struct event_base;

namespace coro_redis {
class client final {
public:
	using awaiter_t = task_awaiter<connection, const redisAsyncContext*>;

    /// 静态单例
	static client& get() {
		static client clt;
		return clt;
	}

	awaiter_t connect(event_base* base, std::string_view host_sv, uint16_t port, long timeout_seconds = 5) {
		return impl_.connect(base, host_sv, port, timeout_seconds);
	}

    //bool create_conn_pool(const std::vector<event_base*> contexts, std::string_view host_sv, uint16_t port, long timeout_seconds = 5);
    //awaiter_t fetch_conn();

private:
	client() = default;
    client(const client&) = delete;
    void operator =(const client&) = delete;

	impl::client_impl impl_;
}; // class client
} // namespace coro_redis

