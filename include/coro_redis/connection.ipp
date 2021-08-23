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
	task_awaiter<CORO_RET> connection::command(std::string_view cmd) const {
		return task_awaiter<CORO_RET>([this, c = std::string(cmd)](task_awaiter<CORO_RET>* awaiter, const coro::coroutine_handle<>& h) {
			char* pcmd = nullptr;
			auto cmd_len = redisFormatCommand(&pcmd, c.c_str());
			auto ret = redisAsyncFormattedCommand(redis_ctx_, [](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
				task_awaiter<CORO_RET>* awaiter = reinterpret_cast<task_awaiter<CORO_RET>*>(pcb_data);
				if (reply) {
					awaiter->set_coro_return((redisReply*)reply);
				}
				awaiter->resume();
				}, awaiter, pcmd, cmd_len);
			redisFreeCommand(pcmd);
		}, [](task_awaiter<CORO_RET>* awaiter, const coro::coroutine_handle<>& h) -> std::optional<CORO_RET> {
			ASSERT_RETURN(awaiter->coro_return().has_value(), std::nullopt, "redis return null.");
			redisReply* pret = std::any_cast<redisReply*>(awaiter->coro_return());
			auto ret = std::unique_ptr<redisReply, decltype(&freeReplyObject)>(pret, freeReplyObject); // auto release
			if (ret->type == REDIS_REPLY_NIL) {
				return std::nullopt;
			}
			if constexpr (std::is_same_v<std::remove_pointer<CORO_RET>::type, redisReply>) {
				return ret;
			}
			else if constexpr (std::is_same_v<CORO_RET, std::string>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_STRING || ret->type == REDIS_REPLY_STATUS, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				return std::string(ret->str, ret->len);
			}
			else if constexpr (std::is_same_v<CORO_RET, uint64_t>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_INTEGER, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				return ret->integer;
			}
			else if constexpr (std::is_void_v<CORO_RET>) {
				return std::nullopt;
			}
			return std::nullopt;
		});
	}
} // namespace coro_redis