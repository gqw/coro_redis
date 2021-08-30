#include "watchdog.h"

#include <sys/types.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#ifdef WIN32
#else
#   include <sys/socket.h>
#   include <unistd.h>
#endif // WIN32


#include "logger.hpp"

#include "reids_client.h"


bool watchdog::start() {
    ASSERT_RETURN(base_ != nullptr, false, "event base is nullptr.");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_);
    listener_ = evconnlistener_new_bind(base_, listener_cb, this,
                                        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                        (struct sockaddr*)&sin, sizeof(sin));

    ASSERT_RETURN(listener_ != nullptr, false, "create socket failed.");
    return true;
}

void watchdog::listener_cb(struct evconnlistener* listener,
                           evutil_socket_t fd,
                           struct sockaddr* remote_addr,
                           int socklen,
                           void* pdata) {
    watchdog* _this = static_cast<watchdog*>(pdata);
    struct event_base* base = (struct event_base*)_this->base_;
    struct bufferevent* bev = nullptr;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        event_base_loopbreak(base);
        printf("Error constructing bufferevent!\n");
        return;
    }
    _this->conn_evts_[fd] = bev;

    bufferevent_setcb(bev, watchdog::read_cb, watchdog::write_cb, watchdog::event_cb, _this);
    bufferevent_enable(bev, EV_WRITE);
    bufferevent_enable(bev, EV_READ);
}

void watchdog::write_cb(struct bufferevent* bev, void* ctx) {

}

void watchdog::event_cb(struct bufferevent* bev, short what, void* ctx) {
    if (what & BEV_EVENT_EOF) {
        LOG_INFO("remote socket closed.");
    } else if (what & BEV_EVENT_ERROR) {
        LOG_INFO("some error, {}", what);
    }

    //这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);
}

task<void> watchdog::coro_call_test_cmds(std::string_view msgs) {
    std::stringstream ss(std::string(msgs.data(), msgs.size()));
    std::string msg;
    while (std::getline(ss, msg, ';')) {
        auto reply = co_await redis_client::get().coro_command(msg);
        if (!reply.has_value()) continue;
        auto r = reply.value();
        if (r) {
            LOG_INFO("echo return, type: {},  integer return: {}, content: {}", r->type, r->integer, std::string(r->str, r->len));
        }
    }
}

task<bool> watchdog::coro_call_test() {
    auto r = co_await redis_client::get().coro_echo("test");
    auto var1 =co_await redis_client::get().coro_incr("var1");
    auto var2 = co_await redis_client::get().coro_incr("var2");
    auto var3 = std::to_string(var1.value() + var2.value());
    co_await redis_client::get().coro_set("var3", var3);
    auto var3_2 = co_await redis_client::get().coro_get("var3");
    LOG_DEBUG("r: {}, var1: {}, var2: {}, var3: {}, var3_2: {}",
              r.value(), var1.value(), var2.value(), var3, var3_2.value());
    co_return true;
}


void watchdog::read_cb(struct bufferevent* bev, void* ctx) {
    watchdog* _this = static_cast<watchdog*>(ctx);
    struct evbuffer* input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    std::string cmds(len, '\0');
    evbuffer_remove(input, cmds.data(), cmds.length());
    LOG_INFO(cmds);
    std::string_view msgs(cmds.data(), len);
    if (msgs.ends_with("\r\n")) {
        msgs.remove_suffix(2);
    }
    LOG_DEBUG("recv msg: {}:{}", msgs.length(), std::string(msgs.data(), len));
    _this->coro_call_test();

    /*redis_client::get().async_command([](struct redisAsyncContext* actx, void* reply, void* pcb_data){
        ASSERT_RETURN(reply != nullptr, void(0), "redis commmand call error, {}({})", actx->errstr, actx->err);
        redisReply* r = (redisReply*)reply;
        LOG_INFO("echo return, type: {},  integer return: {}, content: {}", r->type, r->integer, std::string(r->str, r->len));
    }, _this, msg);*/
}

