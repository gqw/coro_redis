//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#ifdef _USE_SPD_LOG
#   include <logger.hpp>
#else
# ifndef LOG_TRACE
#	define LOG_TRACE(fmt, ...)
# endif
#
# ifndef LOG_DEBUG
#	define LOG_DEBUG(fmt, ...)
# endif
#
# ifndef LOG_INFO
#	define LOG_INFO(fmt, ...)
# endif
#
# ifndef LOG_WARN
#	define LOG_WARN(fmt, ...)
# endif
#
# ifndef LOG_ERROR
#	define LOG_ERROR(fmt, ...)
# endif
#endif // _USE_SPD_LOG


#define ASSERT_RETURN(exp, ret, fmt, ...) do { if(!(exp)) { LOG_ERROR((fmt), ##__VA_ARGS__); return (ret); }}while(false)
#define ASSERT_CO_RETURN(exp, ret, fmt, ...) do { if(!(exp)) { LOG_ERROR((fmt), ##__VA_ARGS__); co_return (ret); }}while(false)

namespace coro_redis {
#ifdef WIN32
namespace coro = std;
#else
namespace coro = std;
#endif // WIN32

using scan_ret_t = std::pair<uint64_t, std::vector<std::string>>;


template <typename T>
struct is_string
    : public std::disjunction<
          std::is_same<char*, typename std::decay_t<T>>,
          std::is_same<const char*, typename std::decay_t<T>>,
          std::is_same<std::string, typename std::decay_t<T>>,
          std::is_same<std::string_view, typename std::decay_t<T>>> {
  constexpr operator bool() const { return true; }
};

template <typename T>
concept RedisSetValueType = std::is_integral<T>::value || is_string<T>::value;

template <typename T>
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

} // namespace coro_redis

