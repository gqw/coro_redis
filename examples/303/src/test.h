#pragma once

#include <string_view>

namespace coro_redis { struct io_context; }
void test_coro_1(const coro_redis::io_context& ioc, std::string_view host, uint16_t port);