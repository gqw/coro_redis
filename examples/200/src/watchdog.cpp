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

void watchdog::read_cb(struct bufferevent* bev, void* ctx) {
    watchdog* _this = static_cast<watchdog*>(ctx);
    struct evbuffer* input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    std::string cmds(len, '\0');
    evbuffer_remove(input, cmds.data(), cmds.length());
    LOG_INFO(cmds);
    std::string_view msg(cmds.data(), len);
    if (msg.ends_with("\r\n")) {
        msg.remove_suffix(2);
    }
    LOG_DEBUG("recv msg: {}:{}", msg.length(), std::string(msg.data(), len));

    redis_client::get().async_command([](struct redisAsyncContext* actx, void* reply, void* pcb_data) {
        ASSERT_RETURN(reply != nullptr, void(0), "redis commmand call error, {}({})", actx->errstr, actx->err);
        redisReply* r = (redisReply*)reply;
        LOG_INFO("echo return, type: {},  integer return: {}, content: {}", r->type, r->integer, std::string(r->str, r->len));
    }, _this, msg);
}

