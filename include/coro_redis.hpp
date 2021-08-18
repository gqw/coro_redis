#pragma once
#include <string>
#include <memory>

#include <hiredis/hiredis.h>

#include "coro_task.h"

#define ASSERT_RETURN(exp, ret, fmt, ...) do { if(!(exp)) { LOG_ERROR((fmt), ##__VA_ARGS__); return (ret); }}while(false)


struct redisAsyncContext;
struct event_base;
struct redisReply;
typedef void (redisCallbackFn)(struct redisAsyncContext*, void*, void*);
class redis_client final {
public:

    // 静态单例
    static redis_client& get() {
        static redis_client clt;
        return clt;
    }
    // 连接
    bool reconnect() {
        auto curtime = time(0);
        if (curtime - last_reconnect_time_ < 5) {
            return true;
        }
        last_reconnect_time_ = curtime;
        return connect(base_, host_, port_);
    }
    bool connect(event_base* base, std::string_view host, uint16_t port) {
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
            }
            else {
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
    void clean() { redis_ctx_ = nullptr; }

    int status() {
        if (redis_ctx_ == nullptr || (redis_ctx_->c.flags & REDIS_CONNECTED) != REDIS_CONNECTED) {
            reconnect();
        }
        return redis_ctx_ ? redis_ctx_->c.flags & REDIS_CONNECTED : REDIS_DISCONNECTING;
    }
    // 发送异步命令
    int async_command(redisCallbackFn* callback, void* pcb_data, std::string_view cmd) {
        ASSERT_RETURN(redis_ctx_ != nullptr, REDIS_ERR, "redis not connect");
        ASSERT_RETURN(status() == REDIS_CONNECTED, REDIS_ERR, "redis not conneted");
        char* pcmd = nullptr;
        auto cmd_len = redisFormatCommand(&pcmd, std::string(cmd).c_str());
        auto ret = redisAsyncFormattedCommand(redis_ctx_, callback, pcb_data, pcmd, cmd_len);
        redisFreeCommand(pcmd);
        ASSERT_RETURN(ret == REDIS_OK, ret, "sync command call failed. cmd{}:{}, {}", cmd.data(), cmd.length(), ret);
        return ret;
    }

    // 发送协程命令

    template<typename CORO_RET = redisReply*>
    task_awaiter<CORO_RET> coro_command(std::string_view cmd) {
        ASSERT_RETURN(status() == REDIS_CONNECTED, (task_awaiter<CORO_RET>()), "redis not conneted");

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
                redisReply* ret = std::any_cast<redisReply*>(awaiter->coro_return());
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

    task_awaiter<std::string> coro_echo(std::string_view msg) {
        return coro_command< std::string>(fmt::format("echo {}", msg));
    }

    task_awaiter<std::string> coro_get(std::string_view key) {
        return coro_command<std::string>(fmt::format("get {}", key));
    }

    task_awaiter<std::string> coro_set(std::string_view key, std::string_view value) {
        return coro_command<std::string>(fmt::format("set {} {}", key, value));
    }
    task_awaiter<uint64_t> coro_incr(std::string_view key) {
        return coro_command<uint64_t>(fmt::format("incr {}", key));
    }


private:
    redis_client() = default;
    redis_client(const redis_client&) = delete;
    redis_client operator =(const redis_client&) = delete;

private:
    event_base* base_ = nullptr;                    // libevent io context
    std::string host_;                              // redis address
    uint16_t port_ = 0;                             // redis port
    redisAsyncContext* redis_ctx_ = nullptr;                  // redis contex
    std::time_t last_reconnect_time_ = 0;
};


