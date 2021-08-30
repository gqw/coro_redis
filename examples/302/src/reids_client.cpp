#include "reids_client.h"

#include <future>
#include <chrono>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "logger.hpp"

using namespace std::chrono_literals;

redis_client& redis_client::get() {
    static redis_client clt;
    return clt;
}

bool redis_client::reconnect() {
    auto curtime = time(0);
    if (curtime - last_reconnect_time_ < 5) {
        return true;
    }
    last_reconnect_time_ = curtime;
    return connect(base_, host_, port_);
}

bool redis_client::connect(event_base* base, std::string_view host, uint16_t port) {
    base_ = base;
    host_ = host;
    port_ = port;
    ASSERT_RETURN(base_, false, "event base is nullptr");
    ASSERT_RETURN(!host.empty(), false, "redis host not set");
    ASSERT_RETURN(port > 0, false, "redis port not set");

    struct timeval timeout = { 50, 500000 }; // 1.5 seconds
    redisOptions opt{};
    std::string sh(host.data(), host.size());
    opt.endpoint.tcp.ip = sh.c_str();
    opt.endpoint.tcp.port = port;
    opt.connect_timeout = &timeout;
    opt.command_timeout = &timeout;
    redisAsyncContext* actx = redisAsyncConnectWithOptions(&opt);
    ASSERT_RETURN(actx != nullptr, false, "connect redis failed, {}:{}", host, port);
    ASSERT_RETURN(actx->err == 0, false, "connect redis failed, {}:{}", actx->err, actx->errstr);
    ASSERT_RETURN(redisLibeventAttach(actx, base) == REDIS_OK, false, "redis event attach failed.");

    redisAsyncSetConnectCallback(actx, [](const struct redisAsyncContext* actx, int status) {
        if (status == REDIS_OK) {
            LOG_INFO("redis connect success.", status);
        } else {
            LOG_ERROR("redis connect error, {}({})", actx->errstr, actx->err);
            redis_client::get().clean();
        }
    });
    redisAsyncSetDisconnectCallback(actx, [](const struct redisAsyncContext* actx, int status) {
        LOG_INFO("redis disconnect status: {}", status);
        redis_client::get().reconnect();

    });
    redis_ctx_ = actx;
    return true;
}

int redis_client::status() {
    if (redis_ctx_ == nullptr || (redis_ctx_->c.flags & REDIS_CONNECTED) != REDIS_CONNECTED) {
        reconnect();
    }
    return redis_ctx_ ? redis_ctx_->c.flags & REDIS_CONNECTED : REDIS_DISCONNECTING;
}

int redis_client::async_command(redisCallbackFn* callback, void* pcb_data, std::string_view cmd) {
    ASSERT_RETURN(redis_ctx_ != nullptr, REDIS_ERR, "redis not connect");
    ASSERT_RETURN(status() == REDIS_CONNECTED, REDIS_ERR, "redis not conneted");
    char* pcmd = nullptr;
    auto cmd_len = redisFormatCommand(&pcmd, std::string(cmd).c_str());
    auto ret = redisAsyncFormattedCommand(redis_ctx_, callback, pcb_data, pcmd, cmd_len);
    redisFreeCommand(pcmd);
    ASSERT_RETURN(ret == REDIS_OK, ret, "sync command call failed. cmd{}:{}, {}", cmd.data(), cmd.length(), ret);
    return ret;
}
