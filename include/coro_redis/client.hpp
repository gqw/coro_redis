//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <hiredis/hiredis.h>
#include <coro_redis/impl/client.ipp>
#include <coro_redis/impl/task.ipp>
#include <coro_redis/context.hpp>
#include <coro_redis/coro_connection.hpp>
#include <coro_redis/sync_connection.hpp>

namespace coro_redis {

///
/// @brief Redis Client
///
class client final {
public:
	using conn_awaiter_t = task_awaiter<std::shared_ptr<coro_connection>, const redisAsyncContext*>;
	using fetch_awaiter_t = task_awaiter<std::shared_ptr<coro_connection>>;
  ///
	/// @brief singleton for get client
	/// 
	static client& get() {
		static client clt;
		return clt;
	}

	///
  /// @brief Connect to redis server with corotine
  /// Example:
  /// @code{.cpp}
  ///   auto conn_ret = co_await client::get().coro_connect(ioc, host, port, 50);
  ///   std::shared_ptr<coro_connection> conn = conn_ret.value(); 
  /// @endcode
	/// @param ioc Wrap for hiredis io context, I.E. libevent, libuv
	/// @param host Redis host name or IP
	/// @param port Reis listening port
	/// @param timeout Connecte and command timeout
	/// @return Corotine expression about connetion
	/// 
	conn_awaiter_t coro_connect(const io_context& ioc, 
		std::string_view host, 
		uint16_t port, 
		long timeout_seconds = 5) {
          return impl_.coro_connect(ioc, host, port, timeout_seconds);
	}

  ///
  /// @brief Connect to redis server synchronously
  ///
  /// @param host Redis host name or IP
  /// @param port Reis listening port
  /// @param timeout Connecte and command timeout
  /// @return Redis synchronous connetion
  /// 
	std::shared_ptr<sync_connection> sync_connect(std::string_view host_sv, 
		uint16_t port, 
		long timeout_seconds = 5) {
		return impl_.sync_connect(host_sv, port, timeout_seconds);
	}

  ///
  /// @brief Initializate redis connect pool
  ///
	/// @param pool_ios IO contexts, pool_ios's size 
	///		is the connection count in pool, the connection
	///   will be lazy initializated at use
  /// @param host Redis host name or IP
  /// @param port Reis listening port
  /// @param timeout Connecte and command timeout
  /// 
	void pool_init(std::vector<io_context*> pool_ios,
		std::string_view host_sv, uint16_t port,
		long timeout_seconds = 5) {
		impl_.pool_init(pool_ios, host_sv, port, timeout_seconds);
	}

	///
  /// @brief Fetch a connection from pool for corotine function
	/// 
	/// @desc This function will look up from unusing list, and then
	///		try create new connection if total connectin is not over max.
	///		If connection pool is full and all connection are using this
	///		function will suspend to wait untill a connection is recycaled.
  /// Example:
  /// @code{.cpp}
  ///   auto conn_ret = co_await client::get().fetch_coro_conn();
  ///   std::shared_ptr<coro_connection> conn = conn_ret.value(); 
  /// @endcode
	/// 
	fetch_awaiter_t fetch_coro_conn() {
		return impl_.fetch_coro_conn();
	}

	///
	/// @brief Fetch a connection from pool for corotine function
	/// 
  /// @desc This function will look up from unusing list, and then
  ///		try create new connection if total connectin is not over
  ///		max. If connection pool is full and all connection are using
  ///		this function will return nullptr.
	/// 
	std::shared_ptr<coro_connection> sync_fetch_conn() {
		return impl_.sync_fetch_conn();
	}

private:
	client() = default;
    client(const client&) = delete;
    void operator =(const client&) = delete;

	impl::client_impl impl_;
}; // class client
} // namespace coro_redis

