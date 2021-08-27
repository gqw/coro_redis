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

template<typename T>
struct is_string
	: public std::disjunction<
	std::is_same<char*, typename std::decay_t<T>>,
	std::is_same<const char*, typename std::decay_t<T>>,
	std::is_same<std::string, typename std::decay_t<T>>,
	std::is_same<std::string_view, typename std::decay_t<T>>
	> {
	constexpr operator bool() const { return true; }
};

template<typename T>
concept RedisSetValueType = std::is_integral<T>::value || is_string<T>::value;

template<typename T>
concept StringValueType = is_string<T>::value;

enum class UpdateType {
	ALWAYS,
	EXIST,
	NOT_EXIST,
	LESS_THAN,
	GREATE_THAN,
	CHANGED,
	INCR,
};
enum class RedisTTLType {
	EX,
	PX,
	EXAT,
	PXAT,
	KEEPTTL,
};

namespace impl {

class connection_impl {
public:
	template <typename CORO_RET>
	using awaiter_t = task_awaiter<CORO_RET, redisReply*>;

	using scan_ret_t = std::pair<uint64_t, std::vector<std::string>>;

	connection_impl(redisAsyncContext* actx) : redis_ctx_(actx) {}

	template<typename CORO_RET>
	awaiter_t<CORO_RET> command(std::string_view cmd) const {
		return awaiter_t<CORO_RET>(
			[this, c = std::string(cmd)](awaiter_t<CORO_RET>* awaiter,
										const coro::coroutine_handle<>& h) {
			char* pcmd = nullptr;
			auto cmd_len = redisFormatCommand(&pcmd, c.c_str());
			auto ret = redisAsyncFormattedCommand(redis_ctx_, [](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
				awaiter_t<CORO_RET>* awaiter = reinterpret_cast<awaiter_t<CORO_RET>*>(pcb_data);
				if (reply) {
					awaiter->set_coro_return((redisReply*)reply);
				}
				awaiter->resume();
				}, awaiter, pcmd, cmd_len);
			redisFreeCommand(pcmd);
		}, [](awaiter_t<CORO_RET>* awaiter, const coro::coroutine_handle<>& h) -> std::optional<CORO_RET> {
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
			if constexpr (std::is_same_v<CORO_RET, std::vector<std::string>>) {
				ASSERT_RETURN(ret->type == REDIS_REPLY_ARRAY, std::nullopt,
					"redis response type not match, {}, {}", ret->type, std::string(ret->str, ret->len));
				std::vector<std::string> resp;
				for (int i = 0; i < ret->elements; ++i)
				{
					if (ret->element[i]->type != REDIS_REPLY_STRING) {
						resp.push_back("");
					}
					else {
						resp.push_back(std::string(ret->element[i]->str, ret->element[i]->len));
					}
				}
				return std::move(resp);
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
	awaiter_t<CORO_RET> command(std::string_view cmd, std::function<std::optional<CORO_RET>(redisReply*)>&& reply_op) const {
		return awaiter_t<CORO_RET>(
			[this, c = std::string(cmd)](awaiter_t<CORO_RET>* awaiter,
				const coro::coroutine_handle<>& h) {
			char* pcmd = nullptr;
			auto cmd_len = redisFormatCommand(&pcmd, c.c_str());
			auto ret = redisAsyncFormattedCommand(redis_ctx_, [](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
				awaiter_t<CORO_RET>* awaiter = reinterpret_cast<awaiter_t<CORO_RET>*>(pcb_data);
				if (reply) {
					awaiter->set_coro_return((redisReply*)reply);
				}
				awaiter->resume();
				}, awaiter, pcmd, cmd_len);
			redisFreeCommand(pcmd);
		}, [op = std::move(reply_op)](awaiter_t<CORO_RET>* awaiter, const coro::coroutine_handle<>& h) -> std::optional<CORO_RET> {
			ASSERT_RETURN(awaiter->coro_return().has_value(), std::nullopt, "redis return null.");
			if (op == nullptr) return std::nullopt;
			return op(awaiter->coro_return().value());
		});
	}

	inline awaiter_t<scan_ret_t> scan(uint64_t cursor,
		std::string_view pattern,
		uint64_t count) {
		std::string cmd("scan");
		cmd.append(" ").append(std::to_string(cursor));
		if (!pattern.empty()) {
			cmd.append(" MATCH ").append(pattern);
		}
		if (count > 0) {
			cmd.append(" COUNT ").append(std::to_string(count));
		}
		return send_scan_cmd(cmd);
	}

	awaiter_t<scan_ret_t> hscan(std::string_view key,
		uint64_t cursor,
		std::string_view pattern,
		uint64_t count) {
		std::string cmd("hscan");
		cmd.append(" ").append(key);
		cmd.append(" ").append(std::to_string(cursor));
		if (!pattern.empty()) {
			cmd.append(" MATCH ").append(pattern);
		}
		if (count > 0) {
			cmd.append(" COUNT ").append(std::to_string(count));
		}
		return send_scan_cmd(cmd);
	}

	awaiter_t<scan_ret_t> sscan(std::string_view key,
		uint64_t cursor,
		std::string_view pattern,
		uint64_t count) {
		std::string cmd("sscan");
		cmd.append(" ").append(key);
		cmd.append(" ").append(std::to_string(cursor));
		if (!pattern.empty()) {
			cmd.append(" MATCH ").append(pattern);
		}
		if (count > 0) {
			cmd.append(" COUNT ").append(std::to_string(count));
		}
		return send_scan_cmd(cmd);
	}

	awaiter_t<scan_ret_t> zscan(std::string_view key,
		uint64_t cursor,
		std::string_view pattern,
		uint64_t count) {
		std::string cmd("zscan");
		cmd.append(" ").append(key);
		cmd.append(" ").append(std::to_string(cursor));
		if (!pattern.empty()) {
			cmd.append(" MATCH ").append(pattern);
		}
		if (count > 0) {
			cmd.append(" COUNT ").append(std::to_string(count));
		}
		return send_scan_cmd(cmd);
	}

	awaiter_t<scan_ret_t> send_scan_cmd(std::string_view cmd) {
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
					ret.second.push_back("");
				}
				else {
					ret.second.push_back(std::string(elem_1->element[i]->str, elem_1->element[i]->len));
				}
			}
			return ret;
			});
	}
	template<RedisSetValueType T>
	awaiter_t<std::string> set(std::string_view key,
		T val,
		uint64_t ttl,
		RedisTTLType ttl_type,
		UpdateType type) {
		std::string cmd(fmt::format("set {} {}", key, val));
		if (ttl > 0 && ttl_type != RedisTTLType::KEEPTTL) {
			switch (ttl_type) {
			case RedisTTLType::EX: cmd.append(" EX "); break;
			case RedisTTLType::PX: cmd.append(" PX "); break;
			case RedisTTLType::EXAT: cmd.append(" EXAT "); break;
			case RedisTTLType::PXAT: cmd.append(" PXAT "); break;
			}
			cmd.append(std::to_string(ttl));
		}
		if (ttl_type == RedisTTLType::KEEPTTL) {
			cmd.append(" KEEPTTL ");
		}
		switch (type) {
		case UpdateType::EXIST: cmd.append(" XX "); break;
		case UpdateType::NOT_EXIST: cmd.append(" NX "); break;
		}
		return command<std::string>(std::move(cmd));
	}

	awaiter_t<uint64_t> hset(std::string_view key, const std::vector<std::pair<std::string_view, std::string_view>>& kvs) {
		std::string cmd("hset");
		cmd.append(" ").append(key);
		for (const auto& kv : kvs)
		{
			cmd.append(" ").append(kv.first);
			cmd.append(" ").append(kv.second);
		}
		return command<uint64_t>(std::move(cmd));
	}

	awaiter_t<uint64_t> zadd(std::string_view key,
		std::string_view member,
		double score,
		UpdateType type) {
		std::string cmd("zadd");
		cmd.append(" ").append(key);
		switch (type)
		{
		case UpdateType::EXIST: cmd.append(" XX ");	break;
		case UpdateType::NOT_EXIST:	cmd.append(" NX "); break;
		case UpdateType::LESS_THAN:	cmd.append(" LT "); break;
		case UpdateType::GREATE_THAN: cmd.append(" GT "); break;
		case UpdateType::CHANGED: cmd.append(" CH "); break;
		case UpdateType::INCR: cmd.append(" INCR "); break;
		}
		cmd.append(" ").append(std::to_string(score));
		cmd.append(" ").append(member);
		return command<uint64_t>(std::move(cmd));
	}

	awaiter_t<uint64_t> zadd(std::string_view key,
		std::vector<std::pair<std::string_view, double>> kvs,
		UpdateType type) {
		std::string cmd("zadd");
		cmd.append(" ").append(key);
		switch (type)
		{
		case UpdateType::EXIST: cmd.append(" XX ");	break;
		case UpdateType::NOT_EXIST:	cmd.append(" NX "); break;
		case UpdateType::LESS_THAN:	cmd.append(" LT "); break;
		case UpdateType::GREATE_THAN: cmd.append(" GT "); break;
		case UpdateType::CHANGED: cmd.append(" CH "); break;
		case UpdateType::INCR: cmd.append(" INCR "); break;
		}
		for (const auto& kv : kvs)
		{
			cmd.append(" ").append(std::to_string(kv.second));
			cmd.append(" ").append(kv.first);
		}
		return command<uint64_t>(std::move(cmd));
	}

	awaiter_t<uint64_t> zunionstore(std::string_view destination, 
		std::initializer_list<std::string_view> keys, std::string_view aggregate) {
		if (keys.size() == 0) return {};

		std::string cmd("zadd");
		cmd.append(" ").append(destination);
		cmd.append(" ").append(std::to_string(keys.size()));
		for (const auto& key : keys)
		{
			cmd.append(" ").append(key);
		}
		cmd.append(" ").append(aggregate);
		return command<uint64_t>(std::move(cmd));
	}

	awaiter_t<uint64_t> zunionstore(std::string_view destination,
		std::vector<std::pair<std::string_view, double>> kvs, std::string_view aggregate) {
		if (kvs.size() == 0) return {};

		std::string cmd("zadd");
		cmd.append(" ").append(destination);
		cmd.append(" ").append(std::to_string(kvs.size()));
		for (const auto& kv : kvs)
		{
			cmd.append(" ").append(kv.first);
		}
		cmd.append(" WEIGHTS ");
		for (const auto& kv : kvs)
		{
			cmd.append(" ").append(std::to_string(kv.second));
		}
		cmd.append(" ").append(aggregate);
		return command<uint64_t>(std::move(cmd));
	}
private:
	redisAsyncContext* redis_ctx_ = nullptr;                  // redis contex
}; // class connection_impl
} // namespace impl
} // namespace coro_redis