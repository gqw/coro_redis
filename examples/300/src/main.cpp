
#include <future>
#include "logger.hpp"
#include "cmdline.hpp"

#include "coro_task.h"

task<> task_test2() {
	co_return;
}
task<int> task_test() {
	int i = 0;
	auto ret = co_await task_awaiter<int>([i](coro::coroutine_handle<task_promise<int>>){
		std::cout << "wait set: " << i << std::endl;
	}, [i]() -> std::optional<int>{
		std::cout << "resume set: " << i+1 << std::endl;
		return i + 1;
	});
	i = *ret;
	ret = co_await task_awaiter<int>([i](coro::coroutine_handle<task_promise<int>>){
		std::cout << "wait set: " << i << std::endl;
	}, [i]() -> std::optional<int>{
		std::cout << "resume set: " << i+1 << std::endl;
		return i + 1;
	});
	i = *ret;
	ret = co_await task_awaiter<int>([i](coro::coroutine_handle<task_promise<int>>){
		std::cout << "wait set: " << i << std::endl;
	}, [i]() -> std::optional<int>{
		std::cout << "resume set: " << i+1 << std::endl;
		return i + 1;
	});
	i = *ret;
	co_return i;
}

int main(int argc, char* argv[]) {
	std::cout << "__cplusplus: " <<  __cplusplus << std::endl;

	{
		auto t2 = task_test2();
		std::cout << "test" << std::endl;
	}

	logger::get().set_level(spdlog::level::trace);
	{
		auto t = task_test();

		t.handler().resume();
		t.handler().resume();
		t.handler().resume();

		std::cout << "final return value: " << (t.return_value()) << std::endl;
		std::cout << "final return value 2: " << (t.return_value()) << std::endl;
	}


	return 0;
}

