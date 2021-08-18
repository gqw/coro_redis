
#include <future>
#include <hiredis/adapters/libevent.h>

#include "logger.hpp"
#include "cmdline.hpp"

#include "reids_client.h"
#include "watchdog.h"
#include "coro_task.h"


int main(int argc, char* argv[]) {
	std::cout << "__cplusplus: "  << __cplusplus << std::endl;

	cmdline::parser cmd;
	cmd.add<std::string>("host", 'h', "redis host name", false, "127.0.0.1");
	cmd.add<uint16_t>("port", 'p', "redis port number", false, 6379, cmdline::range(1, 65535));
	cmd.add<uint16_t>("watchdog", 'w', "watchdog port number", false, 9999, cmdline::range(1, 65535));
	cmd.add<uint16_t>("loglevel", 'l', "set log level", false, spdlog::level::trace, cmdline::range(0, int(spdlog::level::n_levels)));
  	cmd.parse_check(argc, argv);

	auto host = cmd.get<std::string>("host");
	auto port = cmd.get<uint16_t>("port");
	auto watchdog_port = cmd.get<uint16_t>("watchdog");
	auto loglevel = cmd.get<uint16_t>("loglevel");

	// 设置日志级别
	logger::get().set_level(spdlog::level::level_enum(loglevel));
	// 创建libevent上下文环境
	std::shared_ptr<event_base> base(event_base_new(), event_base_free);
	// 打开后台监视窗口
	watchdog dog(base.get(), watchdog_port);
	dog.start();
	// 创建redis连接
	ASSERT_RETURN(redis_client::get().connect(base.get(), host, port), false, "redis connect failed");
	// libevent event reactor
	while (true) {
		event_base_dispatch(base.get());
		std::cout << "run..." << std::endl;
	}

	return 0;
}

