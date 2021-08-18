#pragma once

#include <string>
#include <unordered_map>
#include <hiredis/adapters/libevent.h>

#include "coro_task.h"

struct bufferevent;

class watchdog {
public:
    watchdog(event_base* base, uint16_t port) : base_(base), port_(port) {}

    bool start();

    task<void> coro_call_test_cmds(std::string_view msgs);
    task<void> coro_call_test();

private:
    // void accept();
    static void listener_cb(struct evconnlistener* listener,
        evutil_socket_t fd,
        struct sockaddr* remote_addr,
        int socklen,
        void* pdata);

    static void read_cb(struct bufferevent* bev, void* ctx);
    static void write_cb(struct bufferevent* bev, void* ctx);
    static void event_cb(struct bufferevent* bev, short what, void* ctx);
    
private:
    event_base* base_ = nullptr;                    // libevent io context
    uint16_t port_ = 0;
    struct evconnlistener* listener_;

    std::unordered_map<evutil_socket_t, bufferevent*> conn_evts_;
};