//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
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
} // namespace coro_redis

