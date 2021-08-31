//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <optional>
#include <fmt/format.h>
#include <hiredis/hiredis.h>
#include <coro_redis/impl/config.ipp>

namespace coro_redis {
namespace impl {
class sync_connection_impl {
 public:
  sync_connection_impl(redisContext* ctx) : redis_ctx_(ctx) {}

  template <typename RET>
  static std::optional<RET> deal_redis_reply(redisReply* reply) {
    ASSERT_RETURN(reply != nullptr, std::nullopt, "reply is null");
    ASSERT_RETURN(reply->type != REDIS_REPLY_NIL, std::nullopt,
                  "reply type is null");

    if constexpr (std::is_same_v<RET, std::string>) {
      ASSERT_RETURN(reply->type == REDIS_REPLY_STRING ||
                        reply->type == REDIS_REPLY_STATUS,
                    std::nullopt, "redis response type not match, {}, {}",
                    reply->type, std::string(reply->str, reply->len));
      return std::string(reply->str, reply->len);
    }
    if constexpr (std::is_same_v<RET, std::vector<std::string>>) {
      ASSERT_RETURN(reply->type == REDIS_REPLY_ARRAY, std::nullopt,
                    "redis response type not match, {}, {}", reply->type,
                    std::string(reply->str, reply->len));
      std::vector<std::string> resp;
      for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->type != REDIS_REPLY_STRING) {
          resp.push_back("");
        } else {
          resp.push_back(
              std::string(reply->element[i]->str, reply->element[i]->len));
        }
      }
      return std::move(resp);
    } else if constexpr (std::is_same_v<RET, uint64_t>) {
      ASSERT_RETURN(reply->type == REDIS_REPLY_INTEGER, std::nullopt,
                    "redis response type not match, {}, {}", reply->type,
                    std::string(reply->str, reply->len));
      return reply->integer;
    } else if constexpr (std::is_same_v<RET, double>) {
      ASSERT_RETURN(reply->type == REDIS_REPLY_DOUBLE, std::nullopt,
                    "redis response type not match, {}, {}", reply->type,
                    std::string(reply->str, reply->len));
      return stod(std::string(reply->str, reply->len));
    } else if constexpr (std::is_same_v<RET, bool>) {
      ASSERT_RETURN(reply->type == REDIS_REPLY_BOOL, std::nullopt,
                    "redis response type not match, {}, {}", reply->type,
                    std::string(reply->str, reply->len));
      return stol(std::string(reply->str, reply->len));
    } else if constexpr (std::is_void_v<RET>) {
      return std::nullopt;
    }
    return std::nullopt;
  }

  /// @brief Send redis command.
  /// @param cmd Redis command.
  /// @return Redis return.
  template <typename RET = std::string>
  std::optional<RET> command(std::string_view cmd) const {
    return sync_connection_impl::deal_redis_reply<RET>(
        (redisReply*)redisCommand(redis_ctx_, std::string(cmd).c_str()));
  }

  template <typename RET = std::string>
  std::optional<RET> command(
      std::string_view cmd,
      std::function<std::optional<RET>(redisReply*)> op) const {
    return op((redisReply*)redisCommand(redis_ctx_, std::string(cmd).c_str()));
  }

  inline std::optional<scan_ret_t> scan(uint64_t cursor,
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

  std::optional<scan_ret_t> hscan(std::string_view key, uint64_t cursor,
                                  std::string_view pattern, uint64_t count) {
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

  std::optional<scan_ret_t> sscan(std::string_view key, uint64_t cursor,
                                  std::string_view pattern, uint64_t count) {
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

  std::optional<scan_ret_t> zscan(std::string_view key, uint64_t cursor,
                                  std::string_view pattern, uint64_t count) {
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

  std::optional<scan_ret_t> send_scan_cmd(std::string_view cmd) {
    return command<scan_ret_t>(
        cmd, [](redisReply* reply) -> std::optional<scan_ret_t> {
          scan_ret_t ret;
          ASSERT_RETURN(reply->type == REDIS_REPLY_ARRAY, ret,
                        "scan result type not match, {}", reply->type);
          ASSERT_RETURN(reply->elements == 2, ret,
                        "scan result element count not 2, {}", reply->elements);
          auto elem_0 = reply->element[0];
          ASSERT_RETURN(elem_0->type == REDIS_REPLY_STRING, ret,
                        "scan element[0] type not string, {}", elem_0->type);
          ASSERT_RETURN(elem_0->len > 0, ret, "scan element[0] is null, {}",
                        elem_0->len);
          ret.first = std::stoll(std::string(elem_0->str, elem_0->len));
          auto elem_1 = reply->element[1];
          ASSERT_RETURN(elem_1->type == REDIS_REPLY_ARRAY, ret,
                        "scan element[1] type not match, {}", elem_1->type);
          for (size_t i = 0; i < elem_1->elements; ++i) {
            if (elem_1->element[i]->type != REDIS_REPLY_STRING) {
              ret.second.push_back("");
            } else {
              ret.second.push_back(std::string(elem_1->element[i]->str,
                                               elem_1->element[i]->len));
            }
          }
          return ret;
        });
  }
  template <RedisSetValueType T>
  std::optional<std::string> set(std::string_view key, T val, uint64_t ttl,
                                 RedisTTLType ttl_type, UpdateType type) {
    std::string cmd(fmt::format("set {} {}", key, val));
    if (ttl > 0 && ttl_type != RedisTTLType::KEEPTTL) {
      switch (ttl_type) {
        case RedisTTLType::EX:
          cmd.append(" EX ");
          break;
        case RedisTTLType::PX:
          cmd.append(" PX ");
          break;
        case RedisTTLType::EXAT:
          cmd.append(" EXAT ");
          break;
        case RedisTTLType::PXAT:
          cmd.append(" PXAT ");
          break;
      }
      cmd.append(std::to_string(ttl));
    }
    if (ttl_type == RedisTTLType::KEEPTTL) {
      cmd.append(" KEEPTTL ");
    }
    switch (type) {
      case UpdateType::EXIST:
        cmd.append(" XX ");
        break;
      case UpdateType::NOT_EXIST:
        cmd.append(" NX ");
        break;
    }
    return command<std::string>(std::move(cmd));
  }

  std::optional<uint64_t> hset(
      std::string_view key,
      const std::vector<std::pair<std::string_view, std::string_view>>& kvs) {
    std::string cmd("hset");
    cmd.append(" ").append(key);
    for (const auto& kv : kvs) {
      cmd.append(" ").append(kv.first);
      cmd.append(" ").append(kv.second);
    }
    return command<uint64_t>(std::move(cmd));
  }

  std::optional<uint64_t> zadd(std::string_view key, std::string_view member,
                               double score, UpdateType type) {
    std::string cmd("zadd");
    cmd.append(" ").append(key);
    switch (type) {
      case UpdateType::EXIST: cmd.append(" XX "); break;
      case UpdateType::NOT_EXIST: cmd.append(" NX "); break;
      case UpdateType::LESS_THAN: cmd.append(" LT "); break;
      case UpdateType::GREATE_THAN: cmd.append(" GT "); break;
      case UpdateType::CHANGED: cmd.append(" CH "); break;
      case UpdateType::INCR: cmd.append(" INCR "); break;
      default: break;
    }
    cmd.append(" ").append(std::to_string(score));
    cmd.append(" ").append(member);
    return command<uint64_t>(std::move(cmd));
  }

  std::optional<uint64_t> zadd(
      std::string_view key,
      std::vector<std::pair<std::string_view, double>> kvs, UpdateType type) {
    std::string cmd("zadd");
    cmd.append(" ").append(key);
    switch (type) {
      case UpdateType::EXIST: cmd.append(" XX "); break;
      case UpdateType::NOT_EXIST: cmd.append(" NX "); break;
      case UpdateType::LESS_THAN:  cmd.append(" LT "); break;
      case UpdateType::GREATE_THAN: cmd.append(" GT "); break;
      case UpdateType::CHANGED: cmd.append(" CH ");  break;
      case UpdateType::INCR: cmd.append(" INCR "); break;
      default: break;
    }
    for (const auto& kv : kvs) {
      cmd.append(" ").append(std::to_string(kv.second));
      cmd.append(" ").append(kv.first);
    }
    return command<uint64_t>(std::move(cmd));
  }

  std::optional<uint64_t> zunionstore(
      std::string_view destination,
      std::initializer_list<std::string_view> keys,
      std::string_view aggregate) {
    if (keys.size() == 0) return {};

    std::string cmd("zadd");
    cmd.append(" ").append(destination);
    cmd.append(" ").append(std::to_string(keys.size()));
    for (const auto& key : keys) {
      cmd.append(" ").append(key);
    }
    cmd.append(" ").append(aggregate);
    return command<uint64_t>(std::move(cmd));
  }

  std::optional<uint64_t> zunionstore(
      std::string_view destination,
      std::vector<std::pair<std::string_view, double>> kvs,
      std::string_view aggregate) {
    if (kvs.size() == 0) return {};

    std::string cmd("zadd");
    cmd.append(" ").append(destination);
    cmd.append(" ").append(std::to_string(kvs.size()));
    for (const auto& kv : kvs) {
      cmd.append(" ").append(kv.first);
    }
    cmd.append(" WEIGHTS ");
    for (const auto& kv : kvs) {
      cmd.append(" ").append(std::to_string(kv.second));
    }
    cmd.append(" ").append(aggregate);
    return command<uint64_t>(std::move(cmd));
  }

 private:
  redisContext* redis_ctx_ = nullptr;  // redis contex
};
}  // namespace impl
}  // namespace coro_redis