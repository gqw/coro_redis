#pragma once

#include <string_view>

struct event_base;
void test_coro_1(event_base* base, std::string_view host, uint16_t port);