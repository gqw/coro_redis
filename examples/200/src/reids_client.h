#pragma once
#include <string>
#include <memory>

#define ASSERT_RETURN(exp, ret, fmt, ...) do { if(!(exp)) { LOG_ERROR((fmt), ##__VA_ARGS__); return (ret); }}while(false)

struct redisAsyncContext;
struct event_base;
typedef void (redisCallbackFn)(struct redisAsyncContext*, void*, void*);
class redis_client final {
public:

    // 静态单例
    static redis_client& get();
    // 连接
    bool connect();
    bool connect(event_base* base, std::string_view host, uint16_t port);
    // 发送异步命令
    int async_command(redisCallbackFn* callback, void* pcb_data, std::string_view cmd);
    int async_fmt_command(redisCallbackFn* callback, void* pcb_data, std::string_view fmt, ...);
    void set_status(int status) { status_ = status; }
    int status() { return status_; }

private:
    redis_client() = default;
    redis_client(const redis_client&) = delete;
    redis_client operator =(const redis_client&) = delete;

private:
    event_base* base_ = nullptr;                    // libevent io context
    std::string host_;                              // redis address
    uint16_t port_ = 0;                             // redis port
    std::shared_ptr<redisAsyncContext> redis_ctx_;  // redis contex
    int status_ = 0;
};