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
    static redis_client& get();
    // 连接
    bool reconnect();
    bool connect(event_base* base, std::string_view host, uint16_t port);
    void clean() { redis_ctx_ = nullptr; }
    // 发送异步命令
    int async_command(redisCallbackFn* callback, void* pcb_data, std::string_view cmd);
    // 发送协程命令

    template<typename TASK_RET, typename CORO_RET = redisReply*>
    task_awaiter<TASK_RET, CORO_RET> coro_command(std::string_view cmd) {
        ASSERT_RETURN(status() == REDIS_CONNECTED, (task_awaiter<TASK_RET, CORO_RET>()), "redis not conneted");

        return task_awaiter<TASK_RET, CORO_RET>([this, c = std::string(cmd)](const coro::coroutine_handle<task_promise<TASK_RET>>& h) {
                char* pcmd = nullptr;
                auto cmd_len = redisFormatCommand(&pcmd, c.c_str());
                auto ret = redisAsyncFormattedCommand(redis_ctx_, [](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
                    auto h = coro::coroutine_handle<task_promise<TASK_RET>>::from_address(pcb_data);
                    if (reply) {
                        h.promise().set_coro_return((redisReply*)reply);
                    }
                    h.resume();
                }, h.address(), pcmd, cmd_len);
                redisFreeCommand(pcmd);
            }, [](const coro::coroutine_handle<task_promise<TASK_RET>>& h) -> std::optional<CORO_RET> {
                ASSERT_RETURN(h.promise().coro_return().has_value(), std::nullopt, "redis return null.");
                redisReply* ret = std::any_cast<redisReply*>(h.promise().coro_return());
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

    template<typename TASK_RET = void>
    task_awaiter<TASK_RET, std::string> coro_echo(std::string_view msg) {
        return coro_command<TASK_RET, std::string>(fmt::format("echo {}", msg));
    }

    template<typename TASK_RET = void>
    task_awaiter<TASK_RET, std::string> coro_get(std::string_view key) {
        return coro_command<TASK_RET, std::string>(fmt::format("get {}", key));
    }

    template<typename TASK_RET = void>
    task_awaiter<TASK_RET, std::string> coro_set(std::string_view key, std::string_view value) {
        return coro_command<TASK_RET, std::string>(fmt::format("set {} {}", key, value));
    }
    template<typename TASK_RET = void>
    task_awaiter<TASK_RET, uint64_t> coro_incr(std::string_view key) {
        return coro_command<TASK_RET, uint64_t>(fmt::format("set {} {}", key));
    }
    int status();

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


