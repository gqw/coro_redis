#include "test.h"

#include <coro_redis/client.hpp>

using namespace coro_redis;

task<bool> test_coro_impl(event_base* base, std::string_view host, uint16_t port) {
	auto conn = co_await client::get().connect(base, host, port, 50);
	ASSERT_CO_RETURN(conn.has_value(), false, "connect to redis failed.");

	co_return true;
}

void test_coro_1(event_base* base, std::string_view host, uint16_t port) {
	test_coro_impl(base, host, port);
}
