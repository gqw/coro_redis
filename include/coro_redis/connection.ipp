//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <memory>
#include <coro_redis/task.hpp>

namespace coro_redis {
	template<typename CORO_RET>
	connection::awaiter_t<CORO_RET> connection::command(std::string_view cmd) const {
		return connection::awaiter_t<CORO_RET>(
			[this, c = std::string(cmd)](connection::awaiter_t<CORO_RET>* awaiter,
										const coro::coroutine_handle<>& h) {
			char* pcmd = nullptr;
			auto cmd_len = redisFormatCommand(&pcmd, c.c_str());
			auto ret = redisAsyncFormattedCommand(redis_ctx_, [](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
				connection::awaiter_t<CORO_RET>* awaiter = reinterpret_cast<connection::awaiter_t<CORO_RET>*>(pcb_data);
				if (reply) {
					awaiter->set_coro_return((redisReply*)reply);
				}
				awaiter->resume();
				}, awaiter, pcmd, cmd_len);
			redisFreeCommand(pcmd);
		}, [](connection::awaiter_t<CORO_RET>* awaiter, const coro::coroutine_handle<>& h) -> std::optional<CORO_RET> {
			ASSERT_RETURN(awaiter->coro_return().has_value(), std::nullopt, "redis return null.");
			redisReply* ret = awaiter->coro_return().value();
			if (ret->type == REDIS_REPLY_NIL) {
				return std::nullopt;
			}
			if constexpr (std::is_same_v<CORO_RET, std::string>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_STRING || ret->type == REDIS_REPLY_STATUS, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				return std::string(ret->str, ret->len);
			}
			else if constexpr (std::is_same_v<CORO_RET, uint64_t>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_INTEGER, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				return ret->integer;
			}
			else if constexpr (std::is_same_v<CORO_RET, double>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_DOUBLE, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				return stod(std::string(ret->str, ret->len));
			}
			else if constexpr (std::is_same_v<CORO_RET, bool>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_BOOL, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				return stol(std::string(ret->str, ret->len));
			}
			else if constexpr (std::is_void_v<CORO_RET>) {
				return std::nullopt;
			}
			return std::nullopt;
		});
	}


	template<typename CORO_RET>
	connection::awaiter_t<CORO_RET> connection::command(std::string_view cmd, std::function<std::optional<CORO_RET>(redisReply*)>&& reply_op) const {
		return connection::awaiter_t<CORO_RET>(
			[this, c = std::string(cmd)](connection::awaiter_t<CORO_RET>* awaiter,
				const coro::coroutine_handle<>& h) {
			char* pcmd = nullptr;
			auto cmd_len = redisFormatCommand(&pcmd, c.c_str());
			auto ret = redisAsyncFormattedCommand(redis_ctx_, [](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
				connection::awaiter_t<CORO_RET>* awaiter = reinterpret_cast<connection::awaiter_t<CORO_RET>*>(pcb_data);
				if (reply) {
					awaiter->set_coro_return((redisReply*)reply);
				}
				awaiter->resume();
				}, awaiter, pcmd, cmd_len);
			redisFreeCommand(pcmd);
		}, [op = std::move(reply_op)](connection::awaiter_t<CORO_RET>* awaiter, const coro::coroutine_handle<>& h) -> std::optional<CORO_RET> {
			ASSERT_RETURN(awaiter->coro_return().has_value(), std::nullopt, "redis return null.");
			if (op == nullptr) return std::nullopt;
			return op(awaiter->coro_return().value());
		});
	}

	inline connection::awaiter_t<connection::scan_ret_t> connection::scan(uint64_t cursor,
		std::string_view pattern,
		uint64_t count) {
		std::string cmd("scan ");
		cmd.append(std::to_string(cursor)).append(" ");
		if (!pattern.empty()) {
			cmd.append(" MATCH ").append(pattern);
		}
		if (count > 0) {
			cmd.append(" COUNT ").append(std::to_string(count));
		}
		return command<scan_ret_t>(cmd, [](redisReply* reply) -> std::optional<scan_ret_t> {
			scan_ret_t ret;
			ASSERT_RETURN(reply->type == REDIS_REPLY_ARRAY, ret, "scan result type not match, {}", reply->type);
			ASSERT_RETURN(reply->elements == 2, ret, "scan result element count not 2, {}", reply->elements);
			auto elem_0 = reply->element[0];
			ASSERT_RETURN(elem_0->type == REDIS_REPLY_STRING, ret, "scan element[0] type not string, {}", elem_0->type);
			ASSERT_RETURN(elem_0->len > 0, ret, "scan element[0] is null, {}", elem_0->len);
			ret.first = std::stoll(std::string(elem_0->str, elem_0->len));
			auto elem_1 = reply->element[1];
			ASSERT_RETURN(elem_1->type == REDIS_REPLY_ARRAY, ret, "scan element[1] type not match, {}", elem_1->type);
			for (int i = 0; i < elem_1->elements; ++i)
			{
				if (elem_1->element[i]->type != REDIS_REPLY_STRING) {
					LOG_WARN("scan element not string");
					continue;
				}
				ret.second.push_back(std::string(elem_1->element[i]->str, elem_1->element[i]->len));
			}
			return ret;
			});
	}
} // namespace coro_redis