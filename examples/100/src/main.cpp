#include <future>
#include <chrono>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "logger.hpp"
#include "cmdline.hpp"

using namespace std::chrono_literals;


#define ASSERT_RETURN(exp, ret, fmt, ...) do { if(!(exp)) { LOG_ERROR((fmt), ##__VA_ARGS__); return (ret); }}while(false)


int redis_synccall_test(std::string_view host, uint16_t port) {
    auto ctx = redisConnect(host.data(), port);
    ASSERT_RETURN(ctx != nullptr, 1, "connect redis failed, {}:{}", host, port);
    ASSERT_RETURN(ctx->err == 0, 1, "connect redis failed, {}:{}", ctx->err, ctx->errstr);

    auto r = (redisReply *)redisCommand(ctx, "echo %s", "hello");
    ASSERT_RETURN(r != nullptr, 1, "echo reply is null");
    LOG_INFO("echo return, type: {},  integer return: {}, content: {}", r->type, r->integer, std::string(r->str, r->len));

    redisFree(ctx);
    return 0;
}

int redis_asynccall_test(std::string_view host, uint16_t port) {
    auto actx = redisAsyncConnect(host.data(), port);
    ASSERT_RETURN(actx != nullptr, 1, "connect redis failed, {}:{}", host, port);
    ASSERT_RETURN(actx->err == 0, 1, "connect redis failed, {}:{}", actx->err, actx->errstr);
    struct event_base* base = event_base_new();
    redisLibeventAttach(actx, base);

    redisAsyncSetConnectCallback(actx, [](const struct redisAsyncContext*, int status) {
        LOG_INFO("redis connect status: {}", status);
    });
    redisAsyncSetDisconnectCallback(actx, [](const struct redisAsyncContext* actx, int status) {
        LOG_INFO("redis disconnect status: {}", status);
    });

    redisAsyncCommand(actx, [](struct redisAsyncContext*, void* reply, void*) {
        redisReply* r = (redisReply*)reply;
        LOG_INFO("echo return, type: {},  integer return: {}, content: {}",
                 r->type, r->integer, std::string(r->str, r->len));
    }, nullptr, "echo %s", "hello async");

    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << __cplusplus << std::endl;

    cmdline::parser a;
    a.add<bool>("async", 'a', "use hiredis async interface", false, true);
    a.add<std::string>("host", 'h', "host name", false, "127.0.0.1");
    a.add<uint16_t>("port", 'p', "port number", false, 6379, cmdline::range(1, 65535));

    a.parse_check(argc, argv);

    auto async = a.get<bool>("async");
    auto host = a.get<std::string>("host");
    auto port = a.get<uint16_t>("port");

    std::cout << (async ? "async" : "sync") << ":" << host << ":" << port << std::endl;
    ASSERT_RETURN(!host.empty(), 1, "host cannot be nullptr");

    if (async) {
        redis_asynccall_test(host, port);
    } else {
        redis_synccall_test(host, port);
    }
    LOG_TRACE("test over");

    return 0;
}

