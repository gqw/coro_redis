//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <future>
#include <logger.hpp>
#include <cmdline.hpp>
// #include <hiredis/adapters/libevent.h>
#include <hiredis/adapters/libuv.h>
#include <coro_redis/client.hpp>

#include "test.h"

using namespace coro_redis;

task<bool> coro_redis_test(const io_context& ioc, std::string_view host,
                           uint16_t port) {
    auto conn = co_await client::get().coro_connect(ioc, host, port, 50);
    ASSERT_CO_RETURN(conn != nullptr, false, "connect to redis failed.");

    auto scan_ret = co_await conn->scan(0);
    ASSERT_CO_RETURN(scan_ret.has_value(), false, "call scan failed.");
    auto [cursor, keys] = *scan_ret;
    LOG_TRACE("scan {}", cursor);

    auto echo_ret = co_await conn->echo("hello");
    ASSERT_CO_RETURN(echo_ret.has_value(), false, "call echo failed.");
    LOG_TRACE("echo {}", *echo_ret);

    auto incr_ret = co_await conn->incr("val1");
    ASSERT_CO_RETURN(incr_ret.has_value(), false, "call incr failed.");
    LOG_TRACE("incr {}", *incr_ret);

    auto set_ret = co_await conn->set("val3", 100);
    ASSERT_CO_RETURN(set_ret.has_value(), false, "call set failed.");
    LOG_TRACE("set {}", *set_ret);

    // co_await conn->mset("val1", "100", "val2", 100);
    auto mset_ret = co_await conn->mset("val1", "100", "val2", "100");
    ASSERT_CO_RETURN(mset_ret.has_value(), false, "call mset failed.");
    LOG_TRACE("mset {}", *mset_ret);

    auto mget_ret = co_await conn->mget("val1", "val3", "noexit");
    ASSERT_CO_RETURN(mget_ret.has_value(), false, "call mget failed.");
    LOG_TRACE("mget count: {}", mget_ret->size());

    auto set_ret2 = co_await conn->set("val4", "100");
    ASSERT_CO_RETURN(set_ret2.has_value(), false, "call set failed.");
    LOG_TRACE("set {}", *set_ret2);

    auto get_ret = co_await conn->get("val3");
    ASSERT_CO_RETURN(get_ret.has_value(), false, "call get failed.");
    LOG_TRACE("get {}", *get_ret);

    auto del_cont = co_await conn->del("test", "val1", "val2");
    ASSERT_CO_RETURN(del_cont.has_value(), false, "del keys failed");
    LOG_TRACE("del key count {}", *del_cont);

    ioc.exit();  // exit event dispatch
    co_return true;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersion, &wsaData);
#endif
    // std::cout << "__cplusplus: "  << __cplusplus << std::endl;
    cmdline::parser cmd;
    cmd.add<std::string>("host", 'h', "redis host name", false, "127.0.0.1");
    cmd.add<uint16_t>("port", 'p', "redis port number", false, 6379,
                      cmdline::range(1, 65535));
    cmd.add<uint16_t>("watchdog", 'w', "watchdog port number", false, 9999,
                      cmdline::range(1, 65535));
    cmd.add<uint16_t>("loglevel", 'l', "set log level", false,
                      spdlog::level::trace,
                      cmdline::range(0, int(spdlog::level::n_levels)));
    cmd.parse_check(argc, argv);

    auto host = cmd.get<std::string>("host");
    auto port = cmd.get<uint16_t>("port");
    // auto watchdog_port = cmd.get<uint16_t>("watchdog");
    auto loglevel = cmd.get<uint16_t>("loglevel");

    // ??????????????????
    logger::get().set_level(spdlog::level::level_enum(loglevel));
    // ??????libevent???????????????
    // libevent_io_context ioc;
    libuv_io_context ioc;
    // ????????????????????????
    // watchdog dog(base.get(), watchdog_port);
    // dog.start();
    // ??????redis??????
    // ASSERT_RETURN(coro_redis::get().connect(base.get(), host, port), false,
    // "redis connect failed");

    test_coro_1(ioc, host, port);
    coro_redis_test(ioc, host, port);
    // libevent event reactor
    LOG_INFO("event is run and wait exit...");
    ioc.loop();
    LOG_INFO("exited...");
    return 0;
}
